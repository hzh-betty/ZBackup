#include <utility>

#include "../../include/data/data_manage.h"

namespace zbackup
{
    std::unique_ptr<DataManager> DataManager::instance_;
    std::once_flag DataManager::init_flag_;

    DataManager *DataManager::get_instance()
    {
        std::call_once(init_flag_, []()
        {
            instance_.reset(new DataManager());
        });
        return instance_.get();
    }

    void DataManager::initialize_storage(Storage::ptr storage)
    {
        std::unique_lock<std::shared_mutex> lock(rw_mutex_);
        if (storage_) {
            ZBACKUP_LOG_WARN("DataManager storage already initialized, replacing with new instance");
        }
        storage_ = std::move(storage);
        ZBACKUP_LOG_INFO("DataManager storage initialized successfully");
    }

    bool DataManager::insert(const BackupInfo &info) const
    {
        std::unique_lock<std::shared_mutex> lock(rw_mutex_);
        if (!storage_) {
            ZBACKUP_LOG_ERROR("DataManager storage not initialized");
            return false;
        }
        return storage_->insert(info);
    }

    bool DataManager::update(const BackupInfo &info) const
    {
        std::unique_lock<std::shared_mutex> lock(rw_mutex_);
        if (!storage_) {
            ZBACKUP_LOG_ERROR("DataManager storage not initialized");
            return false;
        }
        return storage_->update(info);
    }

    bool DataManager::get_one_by_url(const std::string &url, BackupInfo *info) const
    {
        std::shared_lock<std::shared_mutex> lock(rw_mutex_);
        if (!storage_) {
            ZBACKUP_LOG_ERROR("DataManager storage not initialized");
            return false;
        }
        return storage_->get_one_by_url(url, info);
    }

    bool DataManager::get_one_by_real_path(const std::string &real_path, BackupInfo *info) const
    {
        std::shared_lock<std::shared_mutex> lock(rw_mutex_);
        if (!storage_) {
            ZBACKUP_LOG_ERROR("DataManager storage not initialized");
            return false;
        }
        return storage_->get_one_by_real_path(real_path, info);
    }

    void DataManager::get_all(std::vector<BackupInfo> *arry) const
    {
        std::shared_lock<std::shared_mutex> lock(rw_mutex_);
        if (!storage_) {
            ZBACKUP_LOG_ERROR("DataManager storage not initialized");
            return;
        }
        storage_->get_all(arry);
    }

    bool DataManager::persistence() const
    {
        std::shared_lock<std::shared_mutex> lock(rw_mutex_);
        if (!storage_) {
            ZBACKUP_LOG_ERROR("DataManager storage not initialized");
            return false;
        }
        return storage_->persistence();
    }

    bool DataManager::delete_one(const BackupInfo &info) const
    {
        std::unique_lock<std::shared_mutex> lock(rw_mutex_);
        if (!storage_) {
            ZBACKUP_LOG_ERROR("DataManager storage not initialized");
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

    bool DataManager::delete_by_url(const std::string &url) const
    {
        std::unique_lock<std::shared_mutex> lock(rw_mutex_);
        if (!storage_) {
            ZBACKUP_LOG_ERROR("DataManager storage not initialized");
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

    bool DataManager::delete_by_real_path(const std::string &real_path) const
    {
        std::unique_lock<std::shared_mutex> lock(rw_mutex_);
        if (!storage_) {
            ZBACKUP_LOG_ERROR("DataManager storage not initialized");
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
}
