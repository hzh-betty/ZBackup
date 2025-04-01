#pragma once
#include <iostream>
#include <mutex>
#include <shared_mutex>
#include <memory>

/*
    读写锁的实现
*/

namespace zbackup
{
    // 读写锁
    class RWMutex
    {
    public:
        RWMutex(const RWMutex &) = delete;
        RWMutex &operator=(const RWMutex &) = delete;
        RWMutex(const RWMutex &&) = delete;
        RWMutex &operator=(const RWMutex &&) = delete;
        RWMutex()
            : mutex_(new std::shared_mutex())
        {
        }
        // 读共享
        void rLock() const
        {
            mutex_->lock_shared();
        }
        void rUnLock() const
        {
            mutex_->unlock_shared();
        }
        // 写互斥
        void wLock()
        {
            mutex_->lock();
        }

        void wUnLock()
        {
            mutex_->unlock();
        }

        ~RWMutex()
        {
            delete mutex_;
        }

    private:
        mutable std::shared_mutex *mutex_;
    };

    class ReadGuard
    {
    public:
        ReadGuard(RWMutex *rMutex)
            : rMutex_(rMutex)
        {
            rMutex_->rLock();
        }
        ~ReadGuard()
        {
            rMutex_->rUnLock();
        }

    private:
        RWMutex *rMutex_;
    };

    class WriteGuard
    {
    public:
        WriteGuard(RWMutex *wMutex)
            : wMutex_(wMutex)
        {
            wMutex_->wLock();
        }
        ~WriteGuard()
        {
            wMutex_->wUnLock();
        }

    private:
        RWMutex *wMutex_;
    };
};
