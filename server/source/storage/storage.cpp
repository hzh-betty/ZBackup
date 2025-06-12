#include "../../include/storage/storage.h"
#include "../../include/core/service_container.h"
#include "../../../ZHttpServer/include/db_pool/mysql_pool.h"
#include "../../include/interfaces/config_manager_interface.h"

#include <memory>

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

        auto& container = core::ServiceContainer::get_instance();
        auto config = container.resolve<interfaces::IConfigManager>();

        std::string pack_dir = config->get_string("pack_dir", "./pack/");
        std::string pack_suffix = config->get_string("packfile_suffix", ".pack");
        std::string down_str = config->get_download_prefix();

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
        auto& container = core::ServiceContainer::get_instance();
        auto config = container.resolve<interfaces::IConfigManager>();
        backup_file_ = config->get_string("backup_file", "./data/backup.dat");
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


    DatabaseStorage::DatabaseStorage()
    {
        auto conn = pool_->get_connection();
        std::string create_table_sql =
            "CREATE TABLE IF NOT EXISTS backup_info ("
            "id INT AUTO_INCREMENT PRIMARY KEY,"
            "pack_flag TINYINT(1) NOT NULL,"
            "fsize BIGINT NOT NULL,"
            "mtime BIGINT NOT NULL,"
            "atime BIGINT NOT NULL,"
            "real_path VARCHAR(50) NOT NULL UNIQUE,"
            "pack_path VARCHAR(50),"
            "url VARCHAR(512) NOT NULL UNIQUE"
            ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;";
        try {
            conn->execute_update(create_table_sql);
        } catch (const std::exception& e) {
            ZBACKUP_LOG_FATAL("Failed to create table backup_info: {}", e.what());
            throw;
        }
        if (!DatabaseStorage::init_load())
        {
            ZBACKUP_LOG_FATAL("Failed to initialize database storage");
            throw std::runtime_error("DatabaseStorage initialization failed");
        }
        ZBACKUP_LOG_INFO("DatabaseStorage initialized successfully");
    }

    bool DatabaseStorage::insert(const BackupInfo &info)
    {
        auto conn = pool_->get_connection();
        std::string sql =
                "INSERT INTO backup_info (pack_flag, fsize, mtime, atime, real_path, pack_path, url) VALUES (?, ?, ?, ?, ?, ?, ?)";
        try
        {
            int ret = conn->execute_update(sql, info.pack_flag_, info.fsize_, info.mtime_, info.atime_, info.real_path_,
                                           info.pack_path_, info.url_);
            if (ret > 0)
            {
                ZBACKUP_LOG_DEBUG("Inserted backup info: {}", info.url_);
                return true;
            }
            else
            {
                ZBACKUP_LOG_ERROR("Failed to insert backup info: {}", info.url_);
                return false;
            }
        }
        catch (const std::exception &e)
        {
            ZBACKUP_LOG_ERROR("Exception on insert: {}", e.what());
            return false;
        }
    }

    bool DatabaseStorage::update(const BackupInfo &info)
    {
        auto conn = pool_->get_connection();
        std::string sql =
                "UPDATE backup_info SET pack_flag=?, fsize=?, mtime=?, atime=?, pack_path=?, url=? WHERE real_path=?";
        try
        {
            int ret = conn->execute_update(sql, info.pack_flag_, info.fsize_, info.mtime_, info.atime_, info.pack_path_,
                                           info.url_, info.real_path_);
            if (ret > 0)
            {
                ZBACKUP_LOG_DEBUG("Updated backup info: {}", info.url_);
                return true;
            }
            else
            {
                ZBACKUP_LOG_ERROR("Failed to update backup info: {}", info.url_);
                return false;
            }
        }
        catch (const std::exception &e)
        {
            ZBACKUP_LOG_ERROR("Exception on update: {}", e.what());
            return false;
        }
    }

    bool DatabaseStorage::get_one_by_url(const std::string &url, BackupInfo *info)
    {
        auto conn = pool_->get_connection();
        std::string sql =
                "SELECT pack_flag, fsize, mtime, atime, real_path, pack_path, url FROM backup_info WHERE url=?";
        try
        {
            auto rows = conn->execute_query(sql, url);
            if (rows.empty())
            {
                ZBACKUP_LOG_WARN("No backup info found for url: {}", url);
                return false;
            }
            const auto &row = rows[0];
            info->pack_flag_ = (row[0] == "1" || row[0] == "true");
            info->fsize_ = std::stoull(row[1]);
            info->mtime_ = std::stoll(row[2]);
            info->atime_ = std::stoll(row[3]);
            info->real_path_ = row[4];
            info->pack_path_ = row[5];
            info->url_ = row[6];
            ZBACKUP_LOG_DEBUG("Got backup info by url: {}", url);
            return true;
        }
        catch (const std::exception &e)
        {
            ZBACKUP_LOG_ERROR("Exception on get_one_by_url: {}", e.what());
            return false;
        }
    }

    bool DatabaseStorage::get_one_by_real_path(const std::string &real_path, BackupInfo *info)
    {
        auto conn = pool_->get_connection();
        std::string sql =
                "SELECT pack_flag, fsize, mtime, atime, real_path, pack_path, url FROM backup_info WHERE real_path=?";
        try
        {
            auto rows = conn->execute_query(sql, real_path);
            if (rows.empty())
            {
                ZBACKUP_LOG_WARN("No backup info found for real_path: {}", real_path);
                return false;
            }
            const auto &row = rows[0];
            info->pack_flag_ = (row[0] == "1" || row[0] == "true");
            info->fsize_ = std::stoull(row[1]);
            info->mtime_ = std::stoll(row[2]);
            info->atime_ = std::stoll(row[3]);
            info->real_path_ = row[4];
            info->pack_path_ = row[5];
            info->url_ = row[6];
            ZBACKUP_LOG_DEBUG("Got backup info by real_path: {}", real_path);
            return true;
        }
        catch (const std::exception &e)
        {
            ZBACKUP_LOG_ERROR("Exception on get_one_by_real_path: {}", e.what());
            return false;
        }
    }

    void DatabaseStorage::get_all(std::vector<BackupInfo> *arry)
    {
        auto conn = pool_->get_connection();
        std::string sql = "SELECT pack_flag, fsize, mtime, atime, real_path, pack_path, url FROM backup_info";
        try
        {
            auto rows = conn->execute_query(sql);
            int count = 0;
            for (const auto &row: rows)
            {
                if (row.size() < 7) continue;
                BackupInfo info;
                info.pack_flag_ = (row[0] == "1" || row[0] == "true");
                info.fsize_ = std::stoull(row[1]);
                info.mtime_ = std::stoll(row[2]);
                info.atime_ = std::stoll(row[3]);
                info.real_path_ = row[4];
                info.pack_path_ = row[5];
                info.url_ = row[6];
                arry->push_back(info);
                ++count;
            }
            ZBACKUP_LOG_DEBUG("Got all backup info, count: {}", count);
        }
        catch (const std::exception &e)
        {
            ZBACKUP_LOG_ERROR("Exception on get_all: {}", e.what());
        }
    }

    bool DatabaseStorage::delete_one(const BackupInfo &info)
    {
        return delete_by_url(info.url_);
    }

    bool DatabaseStorage::delete_by_url(const std::string &url)
    {
        auto conn = pool_->get_connection();
        std::string sql = "DELETE FROM backup_info WHERE url=?";
        try
        {
            int ret = conn->execute_update(sql, url);
            if (ret > 0)
            {
                ZBACKUP_LOG_DEBUG("Deleted backup info by url: {}", url);
                return true;
            }
            else
            {
                ZBACKUP_LOG_ERROR(" Failed to delete backup info by url: {}", url);
                return false;
            }
        }
        catch (const std::exception &e)
        {
            ZBACKUP_LOG_ERROR("Exception on delete_by_url: {}", e.what());
            return false;
        }
    }

    bool DatabaseStorage::delete_by_real_path(const std::string &real_path)
    {
        auto conn = pool_->get_connection();
        std::string sql = "DELETE FROM backup_info WHERE real_path=?";
        try
        {
            int ret = conn->execute_update(sql, real_path);
            if (ret > 0)
            {
                ZBACKUP_LOG_DEBUG("Deleted backup info by real_path: {}", real_path);
                return true;
            }
            else
            {
                ZBACKUP_LOG_ERROR("Failed to delete backup info by real_path: {}", real_path);
                return false;
            }
        }
        catch (const std::exception &e)
        {
            ZBACKUP_LOG_ERROR("Exception on delete_by_real_path: {}", e.what());
            return false;
        }
    }

    bool DatabaseStorage::init_load()
    {
        // 数据库模式无需预加载，返回true

        return true;
    }

    bool DatabaseStorage::persistence()
    {
        // 数据库模式无需持久化，返回true

        return true;
    }
}
