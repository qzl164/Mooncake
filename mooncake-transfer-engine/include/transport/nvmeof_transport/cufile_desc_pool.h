#ifndef CUFILE_DESC_POOL_H_
#define CUFILE_DESC_POOL_H_

#include "transfer_engine.h"
#include <atomic>
#include <bitset>
#include <cstddef>
#include <cstdint>
#include <cufile.h>
#include <mutex>
#include <vector>

namespace mooncake
{
    class CUFileDescPool
    {
    public:
        explicit CUFileDescPool();
        ~CUFileDescPool();

        CUFileDescPool(const CUFileDescPool &) = delete;
        CUFileDescPool &operator=(const CUFileDescPool &) = delete;
        CUFileDescPool(CUFileDescPool &&) = delete;

        int allocCUfileDesc(size_t batch_size); // ret: (desc_idx, start_idx)
        int pushParams(int idx, CUfileIOParams_t &io_params);
        int submitBatch(int idx);
        CUfileIOEvents_t getTransferStatus(int idx, int slice_id);
        int getSliceNum(int idx);
        int freeCUfileDesc(int idx);

    private:
        static const size_t MAX_NR_CUFILE_DESC = 16;
        static const size_t MAX_CUFILE_BATCH_SIZE = 128;
        thread_local static int thread_index;
        static std::atomic<int> index_counter;
        // 1. bitmap, indicates whether a file descriptor is available
        // std::bitset<MAX_NR_CUFILE_DESC> available_;
        // std::bitset<MAX_NR_CUFILE_DESC> available_;
        std::atomic<uint64_t> occupied_[MAX_NR_CUFILE_DESC];
        // 2. cufile desc array
        // CUfileBatchHandle_t handle_[MAX_NR_CUFILE_DESC];
        CUfileBatchHandle_t handle_[MAX_NR_CUFILE_DESC];
        // 3. start idx
        int start_idx_[MAX_NR_CUFILE_DESC];
        // 4. IO Params and IO Status
        std::vector<CUfileIOParams_t> io_params_[MAX_NR_CUFILE_DESC];
        std::vector<CUfileIOEvents_t> io_events_[MAX_NR_CUFILE_DESC];

        RWSpinlock mutex_;
    };

}

#endif