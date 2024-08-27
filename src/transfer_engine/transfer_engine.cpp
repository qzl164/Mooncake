#include "transfer_engine/transfer_engine.h"
#include "transfer_engine/transport.h"
#include "rdma_transport.h"
#ifdef USE_CUDA
#include "nvmeof_transport.h"
#endif

namespace mooncake
{

    int TransferEngine::init(const char *server_name, const char * connectable_name, uint64_t rpc_port) {
        local_server_name_ = server_name;
        // TODO: write to meta server
        return 0;
    }

    int TransferEngine::freeEngine() {
        while (!installed_transports_.empty())
        {
            if (uninstallTransport(installed_transports_.back()->getName()) < 0)
            {
                return -1;
            }
        }
        return 0;
    }

    Transport *TransferEngine::installOrGetTransport(const char* proto, void **args)
    {
        Transport *xport = initTransport(proto);
        if (!xport)
        {
            errno = ENOMEM;
            return NULL;
        }

        if (xport->install(local_server_name_, meta_, args) < 0)
        {
            goto fail;
        }
        installed_transports_.emplace_back(xport);
        return xport;
    fail:
        delete xport;
        return NULL;
    }

    int TransferEngine::uninstallTransport(const char* proto)
    {
        for (auto it = installed_transports_.begin(); it != installed_transports_.end(); ++it)
        {
            if ((*it)->getName() == proto)
            {
                delete *it;
                installed_transports_.erase(it);
                return 0;
            }
        }
        errno = EINVAL;
        return -1;
    }

    Transport::SegmentHandle TransferEngine::openSegment(const char *segment_name)
    {
        // return meta_->getSegmentDesc(segment_name);
        #ifdef USE_LOCAL_DESC
        return 0;
        #else
        return meta_->getSegmentID(segment_name);
        #endif
    }

    int TransferEngine::closeSegment(Transport::SegmentHandle seg_id)
    {
        // TODO
        return 0;
    }

    int TransferEngine::registerLocalMemory(void *addr, size_t length, const std::string &location, bool remote_accessible) {
        for (auto& xport: installed_transports_) {
            if (xport->registerLocalMemory(addr, length, location, remote_accessible) < 0) {
                return -1;
            }
        }
        return 0;
    }

    int TransferEngine::unregisterLocalMemory(void *addr) {
        for (auto &xport : installed_transports_)
        {
            if (xport->unregisterLocalMemory(addr) < 0)
            {
                return -1;
            }
        }
        return 0;
    }

    Transport *TransferEngine::findName(const char *name, size_t n)
    {
        for (const auto &xport : installed_transports_)
        {
            if (strncmp(xport->getName(), name, n) == 0)
            {
                return xport;
            }
        }
        return NULL;
    }

    Transport *TransferEngine::initTransport(const char *proto)
    {
        if (std::string(proto) == "rdma")
        {
            return new RDMATransport();
        }
        #ifdef USE_CUDA
        else if (std::string(proto) == "nvmeof")
        {
            return new NVMeoFTransport();
        }
        #endif
        else
        {
            LOG(ERROR) << "Unsupported Transport Protocol: " << proto;
            return NULL;
        }
    }
}
