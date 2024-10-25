// transfer_engine.h
// Copyright (C) 2024 Feng Ren

#ifndef TRANSFER_ENGINE
#define TRANSFER_ENGINE

#include <infiniband/verbs.h>

#include <atomic>
#include <cstddef>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "transfer_metadata.h"
#include "transport/transport.h"

namespace mooncake {

class RdmaContext;
class RdmaEndPoint;
class TransferMetadata;
class WorkerPool;

class RdmaTransport : public Transport {
    friend class RdmaContext;
    friend class RdmaEndPoint;
    friend class WorkerPool;

   public:
    using BufferDesc = TransferMetadata::BufferDesc;
    using SegmentDesc = TransferMetadata::SegmentDesc;
    using HandShakeDesc = TransferMetadata::HandShakeDesc;

   public:
    RdmaTransport();

    ~RdmaTransport();

    int install(std::string &local_server_name,
                std::shared_ptr<TransferMetadata> meta, void **args) override;

    const char *getName() const override { return "rdma"; }

    int registerLocalMemory(void *addr, size_t length,
                            const std::string &location, bool remote_accessible,
                            bool update_metadata) override;

    int unregisterLocalMemory(void *addr, bool update_metadata = true) override;

    int registerLocalMemoryBatch(const std::vector<BufferEntry> &buffer_list,
                                 const std::string &location) override;

    int unregisterLocalMemoryBatch(
        const std::vector<void *> &addr_list) override;

    // TRANSFER

    int submitTransfer(BatchID batch_id,
                       const std::vector<TransferRequest> &entries) override;

    int getTransferStatus(BatchID batch_id,
                          std::vector<TransferStatus> &status);

    int getTransferStatus(BatchID batch_id, size_t task_id,
                          TransferStatus &status) override;

    SegmentID getSegmentID(const std::string &segment_name);

   private:
    int allocateLocalSegmentID(
        TransferMetadata::PriorityMatrix &priority_matrix);

   public:
    int onSetupRdmaConnections(const HandShakeDesc &peer_desc,
                               HandShakeDesc &local_desc);

    int sendHandshake(const std::string &peer_server_name,
                      const HandShakeDesc &local_desc,
                      HandShakeDesc &peer_desc) {
        return metadata_->sendHandshake(peer_server_name, local_desc,
                                        peer_desc);
    }

   private:
    int initializeRdmaResources();

    int startHandshakeDaemon(std::string &local_server_name);

   public:
    static int selectDevice(SegmentDesc *desc, uint64_t offset, size_t length,
                            int &buffer_id, int &device_id, int retry_cnt = 0);

   private:
    std::vector<std::string> device_name_list_;
    std::vector<std::shared_ptr<RdmaContext>> context_list_;
    std::unordered_map<std::string, int> device_name_to_index_map_;
    std::atomic<SegmentID> next_segment_id_;
};

using TransferRequest = Transport::TransferRequest;
using TransferStatus = Transport::TransferStatus;
using TransferStatusEnum = Transport::TransferStatusEnum;
using SegmentID = Transport::SegmentID;
using BatchID = Transport::BatchID;

}  // namespace mooncake

#endif  // TRANSFER_ENGINE