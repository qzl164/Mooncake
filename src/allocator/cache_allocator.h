#include <atomic>
#include <unordered_map>
#include <map>
#include <vector>
#include <memory>
#include <algorithm>
#include <stdexcept>
#include <random>
#include <iostream>

#include "common_types.h"
#include "virtual_dummy_node.h"
#include "random_allocation_strategy.h"
#include "buffer_allocator.h"

namespace mooncake {

class CacheAllocator
{


public:
    CacheAllocator(size_t shard_size, std::vector<std::unique_ptr<VirtualNode>> nodes, std::unique_ptr<AllocationStrategy> strategy);

    TaskID asyncPut(ObjectKey key, PtrType type, std::vector<void *> ptrs, std::vector<void *> sizes, ReplicateConfig config,  std::vector<TransferRequest>& requests);

    TaskID asyncReplicate(ObjectKey key, ReplicateConfig new_config, ReplicaDiff &replica_diff, std::vector<TransferRequest>& transfer_tasks);

    TaskID asyncGet(ObjectKey key, PtrType type, std::vector<void *> ptrs, std::vector<void *> sizes, Version min_version, size_t offset, std::vector<TransferRequest>& transfer_tasks);

    void RegisterBuffer(std::string type, int segment_id, size_t base, size_t size);

private:
    ReplicaList allocateReplicas(size_t obj_size, int num_replicas);
    void updateObjectMeta(const ObjectKey &key, const ReplicaList &replicas, const ReplicateConfig &config);
    std::pair<Version, ReplicaList> getReplicas(const ObjectKey &key, Version version);
    
     void generateWriteTransferRequests(ReplicaList &replicas, 
            const std::vector<void *> &ptrs, const std::vector<void *> &sizes, int num_replicas, std::vector<TransferRequest>& transfer_requests);

    void generateReadTransferRequests(const std::vector<BufHandle> &replica, 
                                       size_t offset, std::vector<void *> &ptrs, 
                                       const std::vector<void *> &sizes, std::vector<TransferRequest>& transfer_requests);

private:

    // 一个类别有多个 segment
    // 一个 segment 上可能会有多端不连续的 buffer
    // 注意分配失败的回退处理
    BufferResources buf_allocators_; 

    std::atomic<uint64_t> global_version_;
    std::unordered_map<ObjectKey, VersionList> object_meta_;
    // std::vector<std::unique_ptr<VirtualNode>> virtual_nodes_;
    std::unique_ptr<AllocationStrategy> allocation_strategy_;
    size_t shard_size_;
};

} // namespace mooncake
