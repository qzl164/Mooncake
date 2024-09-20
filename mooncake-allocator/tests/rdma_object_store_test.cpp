#include "distributed_object_store.h"
#include <cassert>
#include <cstring>
#include <gtest/gtest.h>
#include <iostream>
#include <malloc.h>
#include <vector>

using namespace mooncake;

class RdmaDistributedObjectStoreTest : public ::testing::Test
{
protected:
    DistributedObjectStore store;
    std::map<SegmentID, std::vector<uint64_t>> segment_and_index;

    void SetUp() override
    {
    //     for (int i = 0; i < 10; i++)
    //     {
    //         for (SegmentID segment_id = 1; segment_id <= 6; segment_id++)
    //         {
    //             segment_and_index[segment_id].push_back(testRegisterBuffer(store, segment_id));
    //         }

    //         // segment_and_index[1] = testRegisterBuffer(store, 1);
    //         // segment_and_index[2] = testRegisterBuffer(store, 2);
    //         // segment_and_index[3] = testRegisterBuffer(store, 3);
    //         // segment_and_index[4] = testRegisterBuffer(store, 4);
    //         // segment_and_index[5] = testRegisterBuffer(store, 5);
    //     }
    }

    void TearDown() override
    {
        // for (auto &meta : segment_and_index)
        // {
        //     // 暂时屏蔽
        //     for (int index = 0; index < meta.second.size(); ++index) {
        //         testUnregisterBuffer(store, meta.first, meta.second[index]);
        //     }
        // }
        LOG(WARNING) << "finish teardown";
    }

    uint64_t testRegisterBuffer(DistributedObjectStore &store, SegmentId segmentId)
    {
        // size_t base = 0x100000000;
        size_t size = 1024 * 1024 * 4 * 200;
        void *ptr = nullptr;
        posix_memalign(&ptr, 4194304, size);
        size_t base = reinterpret_cast<size_t>(ptr);
        LOG(INFO) << "registerbuffer: " << (void *)base;
        uint64_t index = store.registerBuffer(segmentId, base, size);
        EXPECT_GE(index, 0);
        return index;
    }

    void testUnregisterBuffer(DistributedObjectStore &store, SegmentId segmentId, uint64_t index)
    {
        // 会触发recovery 到后面会无法recovery成功
        store.unregisterBuffer(segmentId, index);
    }
};

TEST_F(RdmaDistributedObjectStoreTest, PutGetTest)
{
    ObjectKey key = "test_object";
    
    // 获取本地地址
    std::vector<void *> ptrs;
    //size_t dataSize = 1024 * 1024;
    size_t dataSize = 1024 * 10;
    ptrs.push_back(store.allocateLocalMemory(dataSize));
    std::vector<void *> sizes = {reinterpret_cast<void *>(dataSize)};
    ReplicateConfig config;
    config.replica_num = 1;

    // 在 ptrs[0] 指向的空间中生成随机字符串
    char* dataPtr = static_cast<char*>(ptrs[0]);
    std::generate(dataPtr, dataPtr + dataSize, []() { return 'A' + rand() % 26; });
    dataPtr[dataSize - 1] = '\0';
    // 存储数据
    TaskID putVersion = store.put(key, ptrs, sizes, config);
    EXPECT_NE(putVersion, 0);
    
    LOG(ERROR) << "finish put......";

    // 获取数据
    std::vector<void *> getPtrs;
    getPtrs.push_back(store.allocateLocalMemory(dataSize));
    std::vector<void *> getSizes = {reinterpret_cast<void *>(dataSize)};
    TaskID getVersion = store.get(key, getPtrs, getSizes, 0, 0);
    EXPECT_EQ(getVersion, putVersion);

    // 比较原始数据和获取的数据
    char* retrievedDataPtr = static_cast<char*>(getPtrs[0]);
    retrievedDataPtr[dataSize - 1] = '\0';
    EXPECT_EQ(std::memcmp(dataPtr, retrievedDataPtr, dataSize), 0);
    // LOG(ERROR) << "put data: "  << dataPtr;
    // LOG(ERROR) << "get data: " << retrievedDataPtr;
}
char randomChar()
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis('a', 'z'); // 假设随机字符的范围是 0-255
    return static_cast<char>(dis(gen));
}

void CompareAndLog(const std::vector<char> &combinedPutData, const std::vector<char> &combinedGetData, size_t offset, size_t compareSize)
{
    if (memcmp(combinedPutData.data() + offset, combinedGetData.data(), compareSize) != 0)
    {
        LOG(ERROR) << "Comparison failed: memcmp(combinedPutData.data() + " << offset << ", combinedGetData.data(), " << compareSize << ") != 0";
        LOG(ERROR) << "combinedPutData size: " << combinedPutData.size() - offset << ", content: " << std::string(combinedPutData.data() + offset, compareSize);
        LOG(ERROR) << "combinedGetData size: " << combinedGetData.size() << ",content: " << std::string(combinedGetData.data(), compareSize);
    }
}
int main(int argc, char **argv)
{
    google::InitGoogleLogging("test_log");
    google::SetLogDestination(google::INFO, "logs/log_info_");
    //google::SetLogDestination(google::WARNING, "logs/log_info_");
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
