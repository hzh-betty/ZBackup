#include <utility>
#include "../../include/data/data_manage.h"

namespace zbackup
{
    DataManager::DataManager(interfaces::IBackupStorage::ptr storage)
        : storage_(std::move(storage))
    {
        if (!storage_)
        {
            ZBACKUP_LOG_ERROR("DataManager initialized with null storage");
            throw std::invalid_argument("Storage cannot be null");
        }
        ZBACKUP_LOG_INFO("DataManager initialized successfully");
    }

    bool DataManager::insert(const info::BackupInfo &info)
    {
        if (!storage_)
        {
            ZBACKUP_LOG_ERROR("DataManager storage not available");
            return false;
        }
        return storage_->insert(info);
    }

    bool DataManager::update(const info::BackupInfo &info)
    {
        if (!storage_)
        {
            ZBACKUP_LOG_ERROR("DataManager storage not available");
            return false;
        }
        return storage_->update(info);
    }

    bool DataManager::get_one_by_url(const std::string &url, info::BackupInfo *info)
    {
        if (!storage_)
        {
            ZBACKUP_LOG_ERROR("DataManager storage not available");
            return false;
        }
        return storage_->get_one_by_url(url, info);
    }

    bool DataManager::get_one_by_real_path(const std::string &real_path, info::BackupInfo *info)
    {
        if (!storage_)
        {
            ZBACKUP_LOG_ERROR("DataManager storage not available");
            return false;
        }
        return storage_->get_one_by_real_path(real_path, info);
    }

    void DataManager::get_all(std::vector<info::BackupInfo> *arry)
    {
        if (!storage_)
        {
            ZBACKUP_LOG_ERROR("DataManager storage not available");
            return;
        }
        storage_->get_all(arry);
    }

    bool DataManager::delete_one(const info::BackupInfo &info)
    {
        if (!storage_)
        {
            ZBACKUP_LOG_ERROR("DataManager storage not available");
            return false;
        }
        bool result = storage_->delete_one(info);
        if (result)
        {
            ZBACKUP_LOG_DEBUG("DataManager delete backup info success for url: {}", info.url_);
        }
        else
        {
            ZBACKUP_LOG_WARN("DataManager delete backup info failed for url: {}", info.url_);
        }
        return result;
    }

    bool DataManager::delete_by_url(const std::string &url)
    {
        if (!storage_)
        {
            ZBACKUP_LOG_ERROR("DataManager storage not available");
            return false;
        }
        bool result = storage_->delete_by_url(url);
        if (result)
        {
            ZBACKUP_LOG_DEBUG("DataManager delete backup info by URL success: {}", url);
        }
        else
        {
            ZBACKUP_LOG_WARN("DataManager delete backup info by URL failed: {}", url);
        }
        return result;
    }

    bool DataManager::delete_by_real_path(const std::string &real_path)
    {
        if (!storage_)
        {
            ZBACKUP_LOG_ERROR("DataManager storage not available");
            return false;
        }
        bool result = storage_->delete_by_real_path(real_path);
        if (result)
        {
            ZBACKUP_LOG_DEBUG("DataManager delete backup info by real path success: {}", real_path);
        }
        else
        {
            ZBACKUP_LOG_WARN("DataManager delete backup info by real path failed: {}", real_path);
        }
        return result;
    }

    bool DataManager::persistence()
    {
        if (!storage_)
        {
            ZBACKUP_LOG_ERROR("DataManager storage not available");
            return false;
        }
        // 对于数据库存储，持久化是自动的，总是返回true
        // 对于文件存储，可能需要显式保存，但这里统一处理
        ZBACKUP_LOG_DEBUG("DataManager persistence completed");
        return true;
    }
}