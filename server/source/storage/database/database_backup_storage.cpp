#include "../../../include/storage/database/database_backup_storage.h"
#include "../../../include/log/logger.h"

namespace zbackup::storage
{
    DatabaseBackupStorage::DatabaseBackupStorage()
    {
        create_table_if_not_exists();
        ZBACKUP_LOG_INFO("DatabaseBackupStorage initialized");
    }

    bool DatabaseBackupStorage::insert(const info::BackupInfo &info)
    {
        auto& pool = zhttp::zdb::MysqlConnectionPool::get_instance();
        auto conn = pool.get_connection();
        if (!conn)
        {
            ZBACKUP_LOG_ERROR("Failed to get database connection for insert");
            return false;
        }

        try
        {
            std::string sql = "INSERT INTO backup_files (url, real_path, pack_path, file_size, modify_time, pack_flag) VALUES (?, ?, ?, ?, ?, ?)";
            auto result = conn->execute_update(sql, info.url_, info.real_path_, info.pack_path_, 
                                             info.fsize_, info.mtime_, info.pack_flag_ ? 1 : 0);
            
            if (result > 0)
            {
                ZBACKUP_LOG_DEBUG("Backup info inserted to database: {}", info.url_);
                return true;
            }
            return false;
        }
        catch (const std::exception& e)
        {
            ZBACKUP_LOG_ERROR("Database insert failed: {}", e.what());
            return false;
        }
    }

    bool DatabaseBackupStorage::update(const info::BackupInfo &info)
    {
        auto& pool = zhttp::zdb::MysqlConnectionPool::get_instance();
        auto conn = pool.get_connection();
        if (!conn)
        {
            ZBACKUP_LOG_ERROR("Failed to get database connection for update");
            return false;
        }

        try
        {
            std::string sql = "UPDATE backup_files SET real_path=?, pack_path=?, file_size=?, modify_time=?, pack_flag=? WHERE url=?";
            auto result = conn->execute_update(sql, info.real_path_, info.pack_path_, 
                                             info.fsize_, info.mtime_, info.pack_flag_ ? 1 : 0, info.url_);
            
            if (result > 0)
            {
                ZBACKUP_LOG_DEBUG("Backup info updated in database: {}", info.url_);
                return true;
            }
            return false;
        }
        catch (const std::exception& e)
        {
            ZBACKUP_LOG_ERROR("Database update failed: {}", e.what());
            return false;
        }
    }

    bool DatabaseBackupStorage::get_one_by_id(const std::string &id, info::BackupInfo *info)
    {
        return get_one_by_url(id, info);
    }

    void DatabaseBackupStorage::get_all(std::vector<info::BackupInfo> *arry)
    {
        auto& pool = zhttp::zdb::MysqlConnectionPool::get_instance();
        auto conn = pool.get_connection();
        if (!conn)
        {
            ZBACKUP_LOG_ERROR("Failed to get database connection for get_all");
            return;
        }

        try
        {
            std::string sql = "SELECT url, real_path, pack_path, file_size, modify_time, pack_flag FROM backup_files";
            auto result = conn->execute_query(sql);
            
            arry->clear();
            for (const auto& row : result)
            {
                info::BackupInfo info;
                info.url_ = row[0];
                info.real_path_ = row[1];
                info.pack_path_ = row[2];
                info.fsize_ = std::stoll(row[3]);
                info.mtime_ = std::stoll(row[4]);
                info.pack_flag_ = (row[5] == "1");
                arry->push_back(info);
            }
            
            ZBACKUP_LOG_DEBUG("Retrieved all backup info from database: {} entries", arry->size());
        }
        catch (const std::exception& e)
        {
            ZBACKUP_LOG_ERROR("Database get_all failed: {}", e.what());
        }
    }

    bool DatabaseBackupStorage::delete_one(const info::BackupInfo &info)
    {
        return delete_by_url(info.url_);
    }

    bool DatabaseBackupStorage::delete_by_id(const std::string &id)
    {
        return delete_by_url(id);
    }

    std::vector<info::BackupInfo> DatabaseBackupStorage::find_by_condition(const std::function<bool(const info::BackupInfo&)>& condition)
    {
        std::vector<info::BackupInfo> all_records;
        std::vector<info::BackupInfo> result;
        
        get_all(&all_records);
        for (const auto& record : all_records)
        {
            if (condition(record))
            {
                result.push_back(record);
            }
        }
        return result;
    }

    bool DatabaseBackupStorage::get_one_by_url(const std::string &url, info::BackupInfo *info)
    {
        auto& pool = zhttp::zdb::MysqlConnectionPool::get_instance();
        auto conn = pool.get_connection();
        if (!conn)
        {
            ZBACKUP_LOG_ERROR("Failed to get database connection for get_one_by_url");
            return false;
        }

        try
        {
            std::string sql = "SELECT url, real_path, pack_path, file_size, modify_time, pack_flag FROM backup_files WHERE url=?";
            auto result = conn->execute_query(sql, url);
            
            if (!result.empty())
            {
                const auto& row = result[0];
                info->url_ = row[0];
                info->real_path_ = row[1];
                info->pack_path_ = row[2];
                info->fsize_ = std::stoll(row[3]);
                info->mtime_ = std::stoll(row[4]);
                info->pack_flag_ = (row[5] == "1");
                
                return true;
            }
            
            return false;
        }
        catch (const std::exception& e)
        {
            ZBACKUP_LOG_ERROR("Database get_one_by_url failed: {}", e.what());
            return false;
        }
    }

    bool DatabaseBackupStorage::get_one_by_real_path(const std::string &real_path, info::BackupInfo *info)
    {
        auto& pool = zhttp::zdb::MysqlConnectionPool::get_instance();
        auto conn = pool.get_connection();
        if (!conn)
        {
            ZBACKUP_LOG_ERROR("Failed to get database connection for get_one_by_real_path");
            return false;
        }

        try
        {
            std::string sql = "SELECT url, real_path, pack_path, file_size, modify_time, pack_flag FROM backup_files WHERE real_path=?";
            auto result = conn->execute_query(sql, real_path);
            
            if (!result.empty())
            {
                const auto& row = result[0];
                info->url_ = row[0];
                info->real_path_ = row[1];
                info->pack_path_ = row[2];
                info->fsize_ = std::stoll(row[3]);
                info->mtime_ = std::stoll(row[4]);
                info->pack_flag_ = (row[5] == "1");
                
                return true;
            }
            
            return false;
        }
        catch (const std::exception& e)
        {
            ZBACKUP_LOG_ERROR("Database get_one_by_real_path failed: {}", e.what());
            return false;
        }
    }

    bool DatabaseBackupStorage::delete_by_url(const std::string &url)
    {
        auto& pool = zhttp::zdb::MysqlConnectionPool::get_instance();
        auto conn = pool.get_connection();
        if (!conn)
        {
            ZBACKUP_LOG_ERROR("Failed to get database connection for delete_by_url");
            return false;
        }

        try
        {
            std::string sql = "DELETE FROM backup_files WHERE url=?";
            auto result = conn->execute_update(sql, url);
            
            if (result > 0)
            {
                ZBACKUP_LOG_DEBUG("Backup info deleted from database: {}", url);
                return true;
            }
            return false;
        }
        catch (const std::exception& e)
        {
            ZBACKUP_LOG_ERROR("Database delete_by_url failed: {}", e.what());
            return false;
        }
    }

    bool DatabaseBackupStorage::delete_by_real_path(const std::string &real_path)
    {
        auto& pool = zhttp::zdb::MysqlConnectionPool::get_instance();
        auto conn = pool.get_connection();
        if (!conn)
        {
            ZBACKUP_LOG_ERROR("Failed to get database connection for delete_by_real_path");
            return false;
        }

        try
        {
            std::string sql = "DELETE FROM backup_files WHERE real_path=?";
            auto result = conn->execute_update(sql, real_path);
            
            if (result > 0)
            {
                ZBACKUP_LOG_DEBUG("Backup info deleted from database by real path: {}", real_path);
                return true;
            }
            return false;
        }
        catch (const std::exception& e)
        {
            ZBACKUP_LOG_ERROR("Database delete_by_real_path failed: {}", e.what());
            return false;
        }
    }

    bool DatabaseBackupStorage::create_table_if_not_exists()
    {
        auto& pool = zhttp::zdb::MysqlConnectionPool::get_instance();
        auto conn = pool.get_connection();
        if (!conn)
        {
            ZBACKUP_LOG_ERROR("Failed to get database connection for create table");
            return false;
        }

        try
        {
            std::string sql = R"(
                CREATE TABLE IF NOT EXISTS backup_files (
                    id INT AUTO_INCREMENT PRIMARY KEY,
                    url VARCHAR(128) NOT NULL UNIQUE,
                    real_path VARCHAR(50) NOT NULL,
                    pack_path VARCHAR(50) NOT NULL,
                    file_size BIGINT NOT NULL,
                    modify_time BIGINT NOT NULL,
                    pack_flag BOOLEAN NOT NULL DEFAULT FALSE,
                    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
                    INDEX idx_url (url),
                    INDEX idx_real_path (real_path)
                ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4
            )";
            
            conn->execute_update(sql);
            
            ZBACKUP_LOG_INFO("Backup files table ensured in database");
            return true;
        }
        catch (const std::exception& e)
        {
            ZBACKUP_LOG_ERROR("Failed to create backup files table: {}", e.what());
            return false;
        }
    }
}
