#pragma once
#include "util.hpp"
#include "rw_mutex.hpp"
#include "storage.hpp"

namespace zbackup
{
    class DataManager
    {
    public:
        using ptr = std::shared_ptr<DataManager>;
        DataManager(Storage::ptr storage)
            : storage_(storage)
        {
        }
        // 添加备份信息
        bool insert(const BackupInfo &info)
        {
            WriteGuard wlock(&rwMutex_);
            return storage_->insert(info);
        }

        // 更新备份信息
        bool update(const BackupInfo &info)
        {
            WriteGuard wlock(&rwMutex_);
            return storage_->update(info);
        }

        // 查找备份信息
        bool getOneByURL(const std::string &url, BackupInfo *info)
        {
            ReadGuard rlock(&rwMutex_);
            return storage_->getOneByURL(url, info);
        }

        bool getOneByRealPath(const std::string &realPath, BackupInfo *info)
        {
            ReadGuard rlock(&rwMutex_);
            return storage_->getOneByRealPath(realPath, info);
        }

        void getAll(std::vector<BackupInfo> *arry)
        {
            ReadGuard rlock(&rwMutex_);
            storage_->getAll(arry);
        }

        // 将信息持久化保存
        bool persistence()
        {
            ReadGuard rlock(&rwMutex_);
            return storage_->persistence();
        }

    private:
        RWMutex rwMutex_;      // 读写锁
        Storage::ptr storage_; // 存储方式
    };
};
