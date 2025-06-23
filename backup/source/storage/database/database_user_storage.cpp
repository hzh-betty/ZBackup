/**
 * @file database_user_storage.cpp
 * @brief 数据库用户存储实现，负责用户信息的数据库操作
 */

#include "storage/database/database_user_storage.h"
#include "log/backup_logger.h"


namespace zbackup::storage
{
    /**
     * @brief 构造函数，初始化数据库表
     */
    DatabaseUserStorage::DatabaseUserStorage()
    {
        create_table_if_not_exists();
        ZBACKUP_LOG_INFO("DatabaseUserStorage initialized");
    }

    /**
     * @brief 插入用户信息到数据库
     * @param info 用户信息
     * @return 插入成功返回true，失败返回false
     */
    bool DatabaseUserStorage::insert(const info::UserInfo &info)
    {
        // 1. 获取数据库连接
        auto &pool = zhttp::zdb::MysqlConnectionPool::get_instance();
        auto conn = pool.get_connection();
        if (!conn)
        {
            ZBACKUP_LOG_ERROR("Failed to get database connection for user insert");
            return false;
        }

        try
        {
            // 2. 执行插入SQL，created_at使用当前时间戳
            std::string sql = "INSERT INTO users (username, password_hash, email) VALUES (?, ?, ?)";
            auto result = conn->execute_update(sql, info.username_, info.password_hash_, info.email_);

            if (result > 0)
            {
                ZBACKUP_LOG_DEBUG("User info inserted to database: {}", info.username_);
                return true;
            }
            return false;
        }
        catch (const std::exception &e)
        {
            ZBACKUP_LOG_ERROR("Database user insert failed: {}", e.what());
            return false;
        }
    }

    /**
     * @brief 更新用户信息
     * @param info 用户信息
     * @return 更新成功返回true，失败返回false
     */
    bool DatabaseUserStorage::update(const info::UserInfo &info)
    {
        auto &pool = zhttp::zdb::MysqlConnectionPool::get_instance();
        auto conn = pool.get_connection();
        if (!conn)
        {
            ZBACKUP_LOG_ERROR("Failed to get database connection for user update");
            return false;
        }

        try
        {
            std::string sql = "UPDATE users SET password_hash=?, email=? WHERE username=?";
            auto result = conn->execute_update(sql, info.password_hash_, info.email_, info.username_);

            if (result > 0)
            {
                ZBACKUP_LOG_DEBUG("User info updated in database: {}", info.username_);
                return true;
            }
            return false;
        }
        catch (const std::exception &e)
        {
            ZBACKUP_LOG_ERROR("Database user update failed: {}", e.what());
            return false;
        }
    }

    bool DatabaseUserStorage::get_one_by_id(const std::string &id, info::UserInfo *info)
    {
        return get_by_username(id, info);
    }

    void DatabaseUserStorage::get_all(std::vector<info::UserInfo> *arry)
    {
        auto &pool = zhttp::zdb::MysqlConnectionPool::get_instance();
        auto conn = pool.get_connection();
        if (!conn)
        {
            ZBACKUP_LOG_ERROR("Failed to get database connection for get_all users");
            return;
        }

        try
        {
            std::string sql = "SELECT username, password_hash, email, created_at FROM users";
            auto result = conn->execute_query(sql);

            arry->clear();
            for (const auto &row : result)
            {
                info::UserInfo info;
                info.username_ = row[0];
                info.password_hash_ = row[1];
                info.email_ = row[2];
                info.created_at_ = row[3];
                arry->push_back(info);
            }

            ZBACKUP_LOG_DEBUG("Retrieved all user info from database: {} entries", arry->size());
        }
        catch (const std::exception &e)
        {
            ZBACKUP_LOG_ERROR("Database get_all users failed: {}", e.what());
        }
    }

    bool DatabaseUserStorage::delete_one(const info::UserInfo &info)
    {
        return delete_by_id(info.username_);
    }

    bool DatabaseUserStorage::delete_by_id(const std::string &id)
    {
        auto &pool = zhttp::zdb::MysqlConnectionPool::get_instance();
        auto conn = pool.get_connection();
        if (!conn)
        {
            ZBACKUP_LOG_ERROR("Failed to get database connection for delete user");
            return false;
        }

        try
        {
            std::string sql = "DELETE FROM users WHERE username=?";
            auto result = conn->execute_update(sql, id);

            if (result > 0)
            {
                ZBACKUP_LOG_DEBUG("User deleted from database: {}", id);
                return true;
            }
            return false;
        }
        catch (const std::exception &e)
        {
            ZBACKUP_LOG_ERROR("Database delete user failed: {}", e.what());
            return false;
        }
    }

    std::vector<info::UserInfo> DatabaseUserStorage::find_by_condition(const std::function<bool(const info::UserInfo &)> &condition)
    {
        std::vector<info::UserInfo> all_records;
        std::vector<info::UserInfo> result;

        get_all(&all_records);
        for (const auto &record : all_records)
        {
            if (condition(record))
            {
                result.push_back(record);
            }
        }
        return result;
    }

    /**
     * @brief 根据用户名查询用户信息
     * @param username 用户名
     * @param user 用户信息输出参数
     * @return 查询成功返回true，失败返回false
     */
    bool DatabaseUserStorage::get_by_username(const std::string &username, info::UserInfo *user)
    {
        auto &pool = zhttp::zdb::MysqlConnectionPool::get_instance();
        auto conn = pool.get_connection();
        if (!conn)
        {
            ZBACKUP_LOG_ERROR("Failed to get database connection for get_by_username");
            return false;
        }

        try
        {
            // 查询时包含created_at字段
            std::string sql = "SELECT username, password_hash, email, created_at FROM users WHERE username=?";
            auto result = conn->execute_query(sql, username);

            if (!result.empty())
            {
                const auto &row = result[0];
                user->username_ = row[0];
                user->password_hash_ = row[1];
                user->email_ = row[2];
                user->created_at_ = row[3];

                return true;
            }

            return false;
        }
        catch (const std::exception &e)
        {
            ZBACKUP_LOG_ERROR("Database get_by_username failed: {}", e.what());
            return false;
        }
    }

    /**
     * @brief 根据邮箱查询用户信息
     * @param email 邮箱
     * @param user 用户信息输出参数
     * @return 查询成功返回true，失败返回false
     */
    bool DatabaseUserStorage::get_by_email(const std::string &email, info::UserInfo *user)
    {
        auto &pool = zhttp::zdb::MysqlConnectionPool::get_instance();
        auto conn = pool.get_connection();
        if (!conn)
        {
            ZBACKUP_LOG_ERROR("Failed to get database connection for get_by_email");
            return false;
        }

        try
        {
            std::string sql = "SELECT username, password_hash, email, created_at FROM users WHERE email=?";
            auto result = conn->execute_query(sql, email);

            if (!result.empty())
            {
                const auto &row = result[0];
                user->username_ = row[0];
                user->password_hash_ = row[1];
                user->email_ = row[2];
                user->created_at_ = row[3];

                return true;
            }

            return false;
        }
        catch (const std::exception &e)
        {
            ZBACKUP_LOG_ERROR("Database get_by_email failed: {}", e.what());
            return false;
        }
    }

    /**
     * @brief 创建用户表（如果不存在）
     * @return 创建成功返回true，失败返回false
     */
    bool DatabaseUserStorage::create_table_if_not_exists()
    {
        auto &pool = zhttp::zdb::MysqlConnectionPool::get_instance();
        auto conn = pool.get_connection();
        if (!conn)
        {
            ZBACKUP_LOG_ERROR("Failed to get database connection for create users table");
            return false;
        }

        try
        {
            // 修复表结构：created_at设置默认值为CURRENT_TIMESTAMP
            std::string sql = R"(
                CREATE TABLE IF NOT EXISTS users (
                    id INT AUTO_INCREMENT PRIMARY KEY,
                    username VARCHAR(50) NOT NULL UNIQUE,
                    password_hash VARCHAR(255) NOT NULL,
                    email VARCHAR(100) NOT NULL UNIQUE,
                    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
                    INDEX idx_username (username),
                    INDEX idx_email (email)
                ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4
            )";

            conn->execute_update(sql);

            ZBACKUP_LOG_INFO("Users table ensured in database");
            return true;
        }
        catch (const std::exception &e)
        {
            ZBACKUP_LOG_ERROR("Failed to create users table: {}", e.what());
            return false;
        }
    }
}
