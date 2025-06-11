#include "../../include/storage/storage.h"

namespace zbackup
{
    bool BackupInfo::new_backup_info(const std::string &real_path)
    {
        FileUtil fu(real_path);
        if (fu.exists() == false)
        {
            ZBACKUP_LOG_ERROR("File not exists when creating backup info: {}", real_path);
            return false;
        }

        std::string pack_dir = Config::get_instance().get_pack_dir();
        std::string pack_suffix = Config::get_instance().get_pack_file_prefix();
        std::string down_str = Config::get_instance().get_download_prefix();

        pack_flag_ = false;
        fsize_ = fu.get_size();
        mtime_ = fu.get_last_mtime();
        atime_ = fu.get_last_atime();
        real_path_ = real_path;
        pack_path_ = pack_dir + fu.get_name() + pack_suffix;
        url_ = down_str + fu.get_name();

        ZBACKUP_LOG_DEBUG("Backup info created: {} -> {}", real_path, url_);
        return true;
    }

    FileStorage::FileStorage()
    {
        backup_file_ = Config::get_instance().get_backup_file();
        if (!FileStorage::init_load())
        {
            ZBACKUP_LOG_FATAL("Failed to initialize file storage");
            throw std::runtime_error("FileStorage initialization failed");
        }
        ZBACKUP_LOG_INFO("FileStorage initialized successfully");
    }

    bool FileStorage::insert(const BackupInfo &info)
    {
        tables_[info.url_] = info;
        ZBACKUP_LOG_DEBUG("Backup info inserted: {}", info.url_);
        return true;
    }

    bool FileStorage::update(const BackupInfo &info)
    {
        tables_[info.url_] = info;
        ZBACKUP_LOG_DEBUG("Backup info updated: {}", info.url_);
        return true;
    }

    bool FileStorage::get_one_by_url(const std::string &url, BackupInfo *info)
    {
        auto iter = tables_.find(url);
        if (iter == tables_.end())
        {
            return false;
        }
        *info = iter->second;
        return true;
    }

    bool FileStorage::get_one_by_real_path(const std::string &real_path, BackupInfo *info)
    {
        auto iter = tables_.begin();
        for (; iter != tables_.end(); ++iter)
        {
            if (iter->second.real_path_ == real_path)
            {
                *info = iter->second;
                return true;
            }
        }
        return false;
    }

    void FileStorage::get_all(std::vector<BackupInfo> *arry)
    {
        auto iter = tables_.begin();
        for (; iter != tables_.end(); ++iter)
        {
            arry->push_back(iter->second);
        }
    }

    bool FileStorage::delete_one(const BackupInfo &info)
    {
        auto iter = tables_.find(info.url_);
        if (iter == tables_.end())
        {
            return false;
        }
        tables_.erase(iter);
        ZBACKUP_LOG_DEBUG("Backup info deleted: {}", info.url_);
        return true;
    }

    bool FileStorage::delete_by_url(const std::string &url)
    {
        auto iter = tables_.find(url);
        if (iter == tables_.end())
        {
            return false;
        }
        tables_.erase(iter);
        ZBACKUP_LOG_DEBUG("Backup info deleted by URL: {}", url);
        return true;
    }

    bool FileStorage::delete_by_real_path(const std::string &real_path)
    {
        auto iter = tables_.begin();
        for (; iter != tables_.end(); ++iter)
        {
            if (iter->second.real_path_ == real_path)
            {
                ZBACKUP_LOG_DEBUG("Backup info deleted by real path: {}", real_path);
                tables_.erase(iter);
                return true;
            }
        }
        return false;
    }

    bool FileStorage::init_load()
    {
        FileUtil fu(backup_file_);
        if (fu.exists() == false)
        {
            if (!fu.create_file())
            {
                ZBACKUP_LOG_ERROR("Failed to create backup file: {}", backup_file_);
                return false;
            }
            ZBACKUP_LOG_INFO("Backup file created: {}", backup_file_);
            return true;
        }

        std::string body;
        if (fu.get_content(&body) == false)
        {
            ZBACKUP_LOG_ERROR("Failed to load backup file: {}", backup_file_);
            return false;
        }

        if (body.empty())
        {
            ZBACKUP_LOG_INFO("Backup file is empty, starting with clean state");
            return true;
        }

        nlohmann::json root;
        if (JsonUtil::deserialize(&root, body) == false)
        {
            ZBACKUP_LOG_ERROR("Failed to deserialize backup file: {}", backup_file_);
            return false;
        }

        for (auto &item: root)
        {
            BackupInfo info;
            info.fsize_ = item.value("fsize_", 0);
            info.atime_ = item.value("atime_", 0);
            info.mtime_ = item.value("mtime_", 0);
            info.pack_flag_ = item.value("pack_flag_", false);
            info.real_path_ = item.value("real_path_", "");
            info.pack_path_ = item.value("pack_path_", "");
            info.url_ = item.value("url_", "");
            insert(info);
        }

        ZBACKUP_LOG_INFO("Loaded {} backup entries from file", tables_.size());
        return true;
    }

    bool FileStorage::persistence()
    {
        std::vector<BackupInfo> array;
        get_all(&array);

        nlohmann::json root = nlohmann::json::array();
        for (const auto &info: array)
        {
            nlohmann::json item;
            item["pack_flag_"] = info.pack_flag_;
            item["fsize_"] = info.fsize_;
            item["atime_"] = info.atime_;
            item["mtime_"] = info.mtime_;
            item["real_path_"] = info.real_path_;
            item["pack_path_"] = info.pack_path_;
            item["url_"] = info.url_;
            root.push_back(item);
        }

        std::string body;
        if (JsonUtil::serialize(root, &body) == false)
        {
            ZBACKUP_LOG_ERROR("Failed to serialize backup data");
            return false;
        }

        FileUtil fu(backup_file_);
        if (fu.set_content(body) == false)
        {
            ZBACKUP_LOG_ERROR("Failed to write backup file: {}", backup_file_);
            return false;
        }

        ZBACKUP_LOG_DEBUG("Backup data persisted: {} entries", array.size());
        return true;
    }
}
