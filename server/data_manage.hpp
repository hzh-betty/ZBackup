#pragma once
#include "util.hpp"
#include "storage.hpp"
#include <shared_mutex>
#include <memory>
#include <mutex>

namespace zbackup
{
    class DataManager
    {
    public:
        // 获取单例实例
        static DataManager* getInstance(Storage::ptr storage = nullptr)
        {
            std::call_once(init_flag_, [&storage]() {
                if (!storage) {
                    throw std::runtime_error("Storage must be provided for first initialization");
                }
                instance_.reset(new DataManager(storage));
            });
            return instance_.get();
        }

        // 删除拷贝构造和赋值
        DataManager(const DataManager&) = delete;
        DataManager& operator=(const DataManager&) = delete;

        // 添加备份信息
        bool insert(const BackupInfo &info)
        {
            std::unique_lock<std::shared_mutex> lock(rw_mutex_);
            return storage_->insert(info);
        }

        // 更新备份信息
        bool update(const BackupInfo &info)
        {
            std::unique_lock<std::shared_mutex> lock(rw_mutex_);
            return storage_->update(info);
        }

        // 查找备份信息
        bool getOneByURL(const std::string &url, BackupInfo *info)
        {
            std::shared_lock<std::shared_mutex> lock(rw_mutex_);
            return storage_->getOneByURL(url, info);
        }

        bool getOneByRealPath(const std::string &realPath, BackupInfo *info)
        {
            std::shared_lock<std::shared_mutex> lock(rw_mutex_);
            return storage_->getOneByRealPath(realPath, info);
        }

        void getAll(std::vector<BackupInfo> *arry)
        {
            std::shared_lock<std::shared_mutex> lock(rw_mutex_);
            storage_->getAll(arry);
        }

        // 将信息持久化保存
        bool persistence()
        {
            std::shared_lock<std::shared_mutex> lock(rw_mutex_);
            return storage_->persistence();
        }

        // 删除备份信息
        bool deleteOne(const BackupInfo &info)
        {
            std::unique_lock<std::shared_mutex> lock(rw_mutex_);
            bool result = storage_->deleteOne(info);
            if (result)
            {
                logger->info("DataManager delete backup info success for url[{}]", info.url_);
            }
            else
            {
                logger->warn("DataManager delete backup info failed for url[{}]", info.url_);
            }
            return result;
        }

        // 根据URL删除备份信息
        bool deleteByURL(const std::string &url)
        {
            std::unique_lock<std::shared_mutex> lock(rw_mutex_);
            bool result = storage_->deleteByURL(url);
            if (result)
            {
                logger->info("DataManager delete backup info by URL[{}] success", url);
            }
            else
            {
                logger->warn("DataManager delete backup info by URL[{}] failed", url);
            }
            return result;
        }

        // 根据真实路径删除备份信息
        bool deleteByRealPath(const std::string &realPath)
        {
            std::unique_lock<std::shared_mutex> lock(rw_mutex_);
            bool result = storage_->deleteByRealPath(realPath);
            if (result)
            {
                logger->info("DataManager delete backup info by realPath[{}] success", realPath);
            }
            else
            {
                logger->warn("DataManager delete backup info by realPath[{}] failed", realPath);
            }
            return result;
        }

    private:
        // 私有构造函数
        explicit DataManager(Storage::ptr storage)
            : storage_(storage)
        {
        }

        mutable std::shared_mutex rw_mutex_;      // 读写锁
        Storage::ptr storage_; // 存储方式
        
        static std::unique_ptr<DataManager> instance_;
        static std::once_flag init_flag_;
    };

    // 静态成员定义
    std::unique_ptr<DataManager> DataManager::instance_;
    std::once_flag DataManager::init_flag_;
};
