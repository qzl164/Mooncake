// common.h
// Copyright (C) 2024 Feng Ren

#ifndef COMMON_H
#define COMMON_H

#include <cstdint>
#include <sys/time.h>
#include <glog/logging.h>
#include <sys/time.h>
#include <cstdint>
#include <ctime>
#include <atomic>
#include <sys/mman.h>
#include <numa.h>

#if defined(__x86_64__)
#include <immintrin.h>
#define PAUSE() _mm_pause()
#else
#define PAUSE()
#endif

static inline int64_t get_current_time_nanos()
{
    const int64_t kNanosPerSecond = 1000 * 1000 * 1000;
    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts))
    {
        PLOG(ERROR) << "Failed to read real-time lock";
        return -1;
    }
    return (int64_t{ts.tv_sec} * kNanosPerSecond + int64_t{ts.tv_nsec});
}

static inline ssize_t write_fully(int fd, const void *buf, size_t len)
{
    char *pos = (char *)buf;
    size_t nbytes = len;
    while (nbytes)
    {
        ssize_t rc = write(fd, pos, nbytes);
        if (rc < 0 && (errno == EAGAIN || errno == EINTR))
            continue;
        else if (rc < 0)
        {
            PLOG(ERROR) << "Write failed";
            return rc;
        }
        else if (rc == 0)
            return len - nbytes;
        pos += rc;
        nbytes -= rc;
    }
    return len;
}

static inline ssize_t read_fully(int fd, void *buf, size_t len)
{
    char *pos = (char *)buf;
    size_t nbytes = len;
    while (nbytes)
    {
        ssize_t rc = read(fd, pos, nbytes);
        if (rc < 0 && (errno == EAGAIN || errno == EINTR))
            continue;
        else if (rc < 0)
        {
            PLOG(ERROR) << "Read failed";
            return rc;
        }
        else if (rc == 0)
            return len - nbytes;
        pos += rc;
        nbytes -= rc;
    }
    return len;
}

static inline int write_string(int fd, const std::string &str)
{
    uint64_t length = str.size();
    if (write_fully(fd, &length, sizeof(length)) != (ssize_t)sizeof(length))
        return -1;
    if (write_fully(fd, str.data(), length) != (ssize_t)length)
        return -1;
    return 0;
}

static inline std::string read_string(int fd)
{
    const static size_t kMaxLength = 1ull << 20;
    uint64_t length = 0;
    if (read_fully(fd, &length, sizeof(length)) != (ssize_t)sizeof(length))
        return "";
    if (length > kMaxLength)
        return "";
    std::string str;
    std::vector<char> buffer(length);
    if (read_fully(fd, buffer.data(), length) != (ssize_t)length)
        return "";

    str.assign(buffer.data(), length);
    return str;
}

class RWSpinlock
{
public:
    RWSpinlock() : lock_(0) {}

    ~RWSpinlock() {}

    RWSpinlock(const RWSpinlock &) = delete;

    RWSpinlock &operator=(const RWSpinlock &) = delete;

    void RLock()
    {
        while (true)
        {
            int64_t lock = lock_.fetch_add(1, std::memory_order_relaxed);
            if (lock >= 0)
                break;
            lock_.fetch_sub(1, std::memory_order_relaxed);
        }
        std::atomic_thread_fence(std::memory_order_acquire);
    }

    void RUnlock()
    {
        std::atomic_thread_fence(std::memory_order_release);
        int64_t lock = lock_.fetch_sub(1, std::memory_order_relaxed);
        LOG_ASSERT(lock > 0);
    }

    void WLock()
    {
        while (true)
        {
            int64_t lock;
            while ((lock = lock_.load(std::memory_order_relaxed)))
                PAUSE();
            if (lock_.compare_exchange_weak(lock, kExclusiveLock, std::memory_order_relaxed))
                break;
        }
        std::atomic_thread_fence(std::memory_order_acquire);
    }

    void WUnlock()
    {
        while (true)
        {
            int64_t lock;
            while ((lock = lock_.load(std::memory_order_relaxed)) != kExclusiveLock)
                PAUSE();
            std::atomic_thread_fence(std::memory_order_release);
            if (lock_.compare_exchange_weak(lock, 0, std::memory_order_relaxed))
                return;
        }
    }

    struct WriteGuard
    {
        WriteGuard(RWSpinlock &lock) : lock(lock)
        {
            lock.WLock();
        }

        WriteGuard(const WriteGuard &) = delete;

        WriteGuard &operator=(const WriteGuard &) = delete;

        ~WriteGuard()
        {
            lock.WUnlock();
        }

        RWSpinlock &lock;
    };

    struct ReadGuard
    {
        ReadGuard(RWSpinlock &lock) : lock(lock)
        {
            lock.RLock();
        }

        ReadGuard(const ReadGuard &) = delete;

        ReadGuard &operator=(const ReadGuard &) = delete;

        ~ReadGuard()
        {
            lock.RUnlock();
        }

        RWSpinlock &lock;
    };

private:
    const static int64_t kExclusiveLock = INT64_MIN / 2;

    std::atomic<int64_t> lock_;
    uint64_t padding_[15];
};

#endif // COMMON_H