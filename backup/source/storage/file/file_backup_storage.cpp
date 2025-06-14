#include "storage/file/file_backup_storage.h"
#include "core/service_container.h"
#include "interfaces/config_manager_interface.h"
#include "log/backup_logger.h"
#include "util/util.h"

namespace zbackup::storage
{
    FileBackupStorage::FileBackupStorage()
    {
        auto& container = core::ServiceContainer::get_instance();
        auto config = container.resolve<interfaces::IConfigManager>();
        backup_file_ = config->get_string("backup_file", "./data/backup.dat");
        
        // 确保数据目录存在
        util::FileUtil data_dir("./data/");
        data_dir.create_directory();
        
        init_load();
        ZBACKUP_LOG_INFO("FileBackupStorage initialized with file: {}", backup_file_);
    }

    bool FileBackupStorage::insert(const info::BackupInfo &info)
    {
        std::lock_guard<std::mutex> lock(file_mutex_);
        if (tables_.find(info.url_) != tables_.end())
        {
            ZBACKUP_LOG_WARN("Backup info already exists for URL: {}", info.url_);
            return false;
        }
        
        tables_[info.url_] = info;
        bool result = save_to_file();
        if (result)
        {
            ZBACKUP_LOG_DEBUG("Backup info inserted: {}", info.url_);
        }
        return result;
    }

    bool FileBackupStorage::update(const info::BackupInfo &info)
    {
        std::lock_guard<std::mutex> lock(file_mutex_);
        auto it = tables_.find(info.url_);
        if (it == tables_.end())
        {
            ZBACKUP_LOG_WARN("Backup info not found for update: {}", info.url_);
            return false;
        }
        
        it->second = info;
        bool result = save_to_file();
        if (result)
        {
            ZBACKUP_LOG_DEBUG("Backup info updated: {}", info.url_);
        }
        return result;
    }

    bool FileBackupStorage::get_one_by_id(const std::string &id, info::BackupInfo *info)
    {
        return get_one_by_url(id, info);
    }

    void FileBackupStorage::get_all(std::vector<info::BackupInfo> *arry)
    {
        std::lock_guard<std::mutex> lock(file_mutex_);
        arry->clear();
        for (const auto &pair : tables_)
        {
            arry->push_back(pair.second);
        }
        ZBACKUP_LOG_DEBUG("Retrieved all backup info: {} entries", arry->size());
    }

    bool FileBackupStorage::delete_one(const info::BackupInfo &info)
    {
        return delete_by_url(info.url_);
    }

    bool FileBackupStorage::delete_by_id(const std::string &id)
    {
        return delete_by_url(id);
    }

    std::vector<info::BackupInfo> FileBackupStorage::find_by_condition(const std::function<bool(const info::BackupInfo&)>& condition)
    {
        std::lock_guard<std::mutex> lock(file_mutex_);
        std::vector<info::BackupInfo> result;
        for (const auto &pair : tables_)
        {
            if (condition(pair.second))
            {
                result.push_back(pair.second);
            }
        }
        return result;
    }

    bool FileBackupStorage::get_one_by_url(const std::string &url, info::BackupInfo *info)
    {
        std::lock_guard<std::mutex> lock(file_mutex_);
        auto it = tables_.find(url);
        if (it != tables_.end())
        {
            *info = it->second;
            return true;
        }
        return false;
    }

    bool FileBackupStorage::get_one_by_real_path(const std::string &real_path, info::BackupInfo *info)
    {
        std::lock_guard<std::mutex> lock(file_mutex_);
        for (const auto &pair : tables_)
        {
            if (pair.second.real_path_ == real_path)
            {
                *info = pair.second;
                return true;
            }
        }
        return false;
    }

    bool FileBackupStorage::delete_by_url(const std::string &url)
    {
        std::lock_guard<std::mutex> lock(file_mutex_);
        auto it = tables_.find(url);
        if (it == tables_.end())
        {
            ZBACKUP_LOG_WARN("Backup info not found for deletion: {}", url);
            return false;
        }
        
        tables_.erase(it);
        bool result = save_to_file();
        if (result)
        {
            ZBACKUP_LOG_DEBUG("Backup info deleted: {}", url);
        }
        return result;
    }

    bool FileBackupStorage::delete_by_real_path(const std::string &real_path)
    {
        std::lock_guard<std::mutex> lock(file_mutex_);
        auto it = std::find_if(tables_.begin(), tables_.end(),
            [&real_path](const auto& pair) { return pair.second.real_path_ == real_path; });
        
        if (it == tables_.end())
        {
            ZBACKUP_LOG_WARN("Backup info not found for deletion by real path: {}", real_path);
            return false;
        }
        
        tables_.erase(it);
        bool result = save_to_file();
        if (result)
        {
            ZBACKUP_LOG_DEBUG("Backup info deleted by real path: {}", real_path);
        }
        return result;
    }

    bool FileBackupStorage::init_load()
    {
        util::FileUtil fu(backup_file_);
        if (!fu.exists())
        {
            ZBACKUP_LOG_INFO("Backup file not found, starting with empty storage: {}", backup_file_);
            return true;
        }

        std::string body;
        if (!fu.get_content(&body))
        {
            ZBACKUP_LOG_ERROR("Failed to read backup file: {}", backup_file_);
            return false;
        }

        nlohmann::json root;
        if (!util::JsonUtil::deserialize(&root, body))
        {
            ZBACKUP_LOG_ERROR("Failed to parse backup file: {}", backup_file_);
            return false;
        }

        for (const auto& item : root)
        {
            info::BackupInfo bi;
            bi.url_ = item["url"];
            bi.real_path_ = item["real_path"];
            bi.pack_path_ = item["pack_path"];
            bi.fsize_ = item["fsize"];
            bi.mtime_ = item["mtime"];
            bi.pack_flag_ = item["pack_flag"];
            tables_[bi.url_] = bi;
        }

        ZBACKUP_LOG_INFO("Loaded {} backup entries from file", tables_.size());
        return true;
    }

    bool FileBackupStorage::save_to_file()
    {
        nlohmann::json root = nlohmann::json::array();
        
        for (const auto& pair : tables_)
        {
            const auto& bi = pair.second;
            nlohmann::json item;
            item["url"] = bi.url_;
            item["real_path"] = bi.real_path_;
            item["pack_path"] = bi.pack_path_;
            item["fsize"] = bi.fsize_;
            item["mtime"] = bi.mtime_;
            item["pack_flag"] = bi.pack_flag_;
            root.push_back(item);
        }

        std::string body;
        if (!util::JsonUtil::serialize(root, &body))
        {
            ZBACKUP_LOG_ERROR("Failed to serialize backup data");
            return false;
        }

        util::FileUtil fu(backup_file_);
        if (!fu.set_content(body))
        {
            ZBACKUP_LOG_ERROR("Failed to write backup file: {}", backup_file_);
            return false;
        }

        return true;
    }
}
