#include "transfer_engine/endpoint_store.h"
#include "transfer_engine/rdma_context.h"
#include "transfer_engine/rdma_endpoint.h"
#include "transfer_engine/config.h"
#include <atomic>
#include <cassert>
#include <cstddef>
#include <glog/logging.h>
#include <memory>
#include <utility>

namespace mooncake
{
    std::shared_ptr<RdmaEndPoint> FIFOEndpointStore::getEndpoint(std::string peer_nic_path)
    {
        RWSpinlock::ReadGuard guard(endpoint_map_lock_);
        auto iter = endpoint_map_.find(peer_nic_path);
        if (iter != endpoint_map_.end())
            return iter->second;
        return nullptr;
    }

    std::shared_ptr<RdmaEndPoint> FIFOEndpointStore::insertEndpoint(std::string peer_nic_path, RdmaContext *context)
    {
        RWSpinlock::WriteGuard guard(endpoint_map_lock_);
        if (endpoint_map_.find(peer_nic_path) != endpoint_map_.end())
        {
            LOG(INFO) << "Endpoint " << peer_nic_path << " already exists in FIFOEndpointStore";
            return endpoint_map_[peer_nic_path];
        }
        auto endpoint = std::make_shared<RdmaEndPoint>(*context);
        if (!endpoint)
        {
            PLOG(ERROR) << "Failed to allocate memory for RdmaEndPoint";
            return nullptr;
        }
        auto &config = globalConfig();
        int ret = endpoint->construct(context->cq(), 
                                      config.num_qp_per_ep,
                                      config.max_sge,
                                      config.max_wr,
                                      config.max_inline);
        if (ret)
            return nullptr;

        while (this->getSize() >= max_size_)
            evictEndpoint();

        endpoint->setPeerNicPath(peer_nic_path);
        endpoint_map_[peer_nic_path] = endpoint;
        fifo_list_.push_back(peer_nic_path);
        auto it = fifo_list_.end();
        fifo_map_[peer_nic_path] = --it;
        return endpoint;
    }

    int FIFOEndpointStore::deleteEndpoint(std::string peer_nic_path)
    {
        RWSpinlock::WriteGuard guard(endpoint_map_lock_);
        auto iter = endpoint_map_.find(peer_nic_path);
        if (iter != endpoint_map_.end())
        {
            endpoint_map_.erase(iter);
            auto fifo_iter = fifo_map_[peer_nic_path];
            fifo_list_.erase(fifo_iter);
            fifo_map_.erase(peer_nic_path);
        }
        return 0;
    }

    void FIFOEndpointStore::evictEndpoint()
    {
        if (fifo_list_.empty())
            return;
        std::string victim = fifo_list_.front();
        fifo_list_.pop_front();
        fifo_map_.erase(victim);
        LOG(INFO) << victim << " evicted";
        endpoint_map_.erase(victim);
        return;
    }

    size_t FIFOEndpointStore::getSize()
    {
        return endpoint_map_.size();
    }

    int FIFOEndpointStore::destroyQPs() {
        for (auto &kv : endpoint_map_) {
            kv.second->destroyQP();
        }
        return 0;
    }

    std::shared_ptr<RdmaEndPoint> SIEVEEndpointStore::getEndpoint(std::string peer_nic_path)
    {
        RWSpinlock::ReadGuard guard(endpoint_map_lock_);
        auto iter = endpoint_map_.find(peer_nic_path);
        if (iter != endpoint_map_.end())
        {
            iter->second.second.store(true, std::memory_order_relaxed); // This is safe within read lock because of idempotence
            return iter->second.first;
        }
        LOG(INFO) << "Endpoint " << peer_nic_path << " not found in SIEVEEndpointStore";
        return nullptr;
    }

    std::shared_ptr<RdmaEndPoint> SIEVEEndpointStore::insertEndpoint(std::string peer_nic_path, RdmaContext *context)
    {
        RWSpinlock::WriteGuard guard(endpoint_map_lock_);
        if (endpoint_map_.find(peer_nic_path) != endpoint_map_.end())
        {
            LOG(INFO) << "Endpoint " << peer_nic_path << " already exists in FIFOEndpointStore";
            return endpoint_map_[peer_nic_path].first;
        }
        auto endpoint = std::make_shared<RdmaEndPoint>(*context);
        if (!endpoint)
        {
            PLOG(ERROR) << "Failed to allocate memory for RdmaEndPoint";
            return nullptr;
        }
        auto &config = globalConfig();
        int ret = endpoint->construct(context->cq(), 
                                      config.num_qp_per_ep,
                                      config.max_sge,
                                      config.max_wr,
                                      config.max_inline);
        if (ret) 
            return nullptr;

        while (this->getSize() >= max_size_)
            evictEndpoint();

        endpoint->setPeerNicPath(peer_nic_path);
        endpoint_map_[peer_nic_path] = std::make_pair(endpoint, false);
        fifo_list_.push_front(peer_nic_path);
        fifo_map_[peer_nic_path] = fifo_list_.begin();
        return endpoint;
    }

    int SIEVEEndpointStore::deleteEndpoint(std::string peer_nic_path)
    {
        RWSpinlock::WriteGuard guard(endpoint_map_lock_);
        auto iter = endpoint_map_.find(peer_nic_path);
        if (iter != endpoint_map_.end())
        {
            endpoint_map_.erase(iter);
            auto fifo_iter = fifo_map_[peer_nic_path];
            if (hand_.has_value() && hand_.value() == fifo_iter)
            {
                fifo_iter == fifo_list_.begin() ? hand_ = std::nullopt : hand_ = std::prev(fifo_iter);
            }
            fifo_list_.erase(fifo_iter);
            fifo_map_.erase(peer_nic_path);
        }
        return 0;
    }

    void SIEVEEndpointStore::evictEndpoint()
    {
        if (fifo_list_.empty())
        {
            return;
        }
        auto o = hand_.has_value() ? hand_.value() : --fifo_list_.end();
        std::string victim;
        while (true)
        {
            victim = *o;
            if (endpoint_map_[victim].second.load(std::memory_order_relaxed))
            {
                endpoint_map_[victim].second.store(false, std::memory_order_relaxed);
                o = (o == fifo_list_.begin() ? --fifo_list_.end() : std::prev(o));
            }
            else
            {
                break;
            }
        }
        hand_ = (o == fifo_list_.begin() ? --fifo_list_.end() : std::prev(o));
        fifo_list_.erase(o);
        fifo_map_.erase(victim);
        LOG(INFO) << victim << " evicted";
        endpoint_map_.erase(victim);
        return;
    }

    int SIEVEEndpointStore::destroyQPs() {
        for (auto &kv : endpoint_map_) {
            kv.second.first->destroyQP();
        }
        return 0;
    }

    size_t SIEVEEndpointStore::getSize()
    {
        return endpoint_map_.size();
    }
}