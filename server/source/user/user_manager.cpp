#include "../../include/user/user_manager.h"
#include "../../include/config/config.h"
#include "../../include/log/logger.h"
#include <iomanip>
#include <sstream>
#include <functional>

namespace zbackup
{
    UserManager::UserManager() : mysql_pool_(zhttp::zdb::MysqlConnectionPool::get_instance())
    {
        // 从配置获取数据库连接信息
        Config& config = Config::get_instance();
        
        // 使用单例连接池的init方法初始化
        mysql_pool_.init(
            config.get_db_host(),
            config.get_db_user(), 
            config.get_db_password(),
            config.get_db_name(),
            10  // 连接池大小
        );
        
        if (!mysql_pool_.is_initialized()) {
            ZBACKUP_LOG_FATAL("Failed to initialize MySQL connection pool");
            throw std::runtime_error("Failed to initialize MySQL connection pool");
        }
        
        // 初始化用户表
        if (!init_user_table()) {
            ZBACKUP_LOG_ERROR("Failed to initialize user table");
        }
        
        ZBACKUP_LOG_INFO("UserManager initialized successfully");
    }

    bool UserManager::init_user_table()
    {
        auto conn = mysql_pool_.get_connection();
        if (!conn) {
            ZBACKUP_LOG_ERROR("Failed to get database connection");
            return false;
        }

        const std::string create_table_sql = R"(
            CREATE TABLE IF NOT EXISTS users (
                id INT AUTO_INCREMENT PRIMARY KEY,
                username VARCHAR(50) UNIQUE NOT NULL,
                password_hash VARCHAR(128) NOT NULL,
                email VARCHAR(100) NOT NULL,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                is_active BOOLEAN DEFAULT TRUE,
                INDEX idx_username (username)
            ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
        )";

        try {
            auto result = conn->execute_update(create_table_sql);
            ZBACKUP_LOG_INFO("Users table initialized successfully");
            return true;
        } catch (const std::exception& e) {
            ZBACKUP_LOG_ERROR("Exception creating users table: {}", e.what());
            return false;
        }
    }

    bool UserManager::register_user(const std::string& username, const std::string& password, const std::string& email)
    {
        if (username.empty() || password.empty() || email.empty()) {
            ZBACKUP_LOG_WARN("Register failed: empty username, password or email");
            return false;
        }

        auto conn = mysql_pool_.get_connection();
        if (!conn) {
            ZBACKUP_LOG_ERROR("Failed to get database connection for registration");
            return false;
        }

        try {
            // 检查用户名是否已存在
            std::string check_sql = "SELECT username FROM users WHERE username = ?";
            auto result = conn->execute_query(check_sql, username);
            
            if (!result.empty()) {
                ZBACKUP_LOG_WARN("Registration failed: username '{}' already exists", username);
                return false;
            }

            // 插入新用户
            std::string password_hash = hash_password(password);
            std::string insert_sql = "INSERT INTO users (username, password_hash, email) VALUES (?, ?, ?)";
            
            int affected_rows = conn->execute_update(insert_sql, username, password_hash, email);
            if (affected_rows <= 0) {
                ZBACKUP_LOG_ERROR("Failed to insert user");
                return false;
            }

            ZBACKUP_LOG_INFO("User '{}' registered successfully", username);
            return true;
        } catch (const std::exception& e) {
            ZBACKUP_LOG_ERROR("Exception during user registration: {}", e.what());
            return false;
        }
    }

    bool UserManager::validate_user(const std::string& username, const std::string& password)
    {
        if (username.empty() || password.empty()) {
            return false;
        }

        auto conn = mysql_pool_.get_connection();
        if (!conn) {
            ZBACKUP_LOG_ERROR("Failed to get database connection for validation");
            return false;
        }

        try {
            std::string sql = "SELECT password_hash, is_active FROM users WHERE username = ?";
            auto result = conn->execute_query(sql, username);
            
            if (result.empty()) {
                ZBACKUP_LOG_WARN("Login failed: user '{}' not found", username);
                return false;
            }

            // result是二维向量，result[0]是第一行，result[0][0]是第一列
            std::string stored_hash = result[0][0];  // password_hash列
            std::string is_active_str = result[0][1];  // is_active列
            bool is_active = (is_active_str == "1" || is_active_str == "true");

            if (!is_active) {
                ZBACKUP_LOG_WARN("Login failed: user '{}' is disabled", username);
                return false;
            }

            bool valid = verify_password(password, stored_hash);
            if (valid) {
                ZBACKUP_LOG_INFO("User '{}' logged in successfully", username);
            } else {
                ZBACKUP_LOG_WARN("Login failed: invalid password for user '{}'", username);
            }

            return valid;
        } catch (const std::exception& e) {
            ZBACKUP_LOG_ERROR("Exception during user validation: {}", e.what());
            return false;
        }
    }

    std::optional<UserInfo> UserManager::get_user_info(const std::string& username)
    {
        auto conn = mysql_pool_.get_connection();
        if (!conn) {
            ZBACKUP_LOG_ERROR("Failed to get database connection");
            return std::nullopt;
        }

        try {
            std::string sql = "SELECT username, password_hash, email, created_at, is_active FROM users WHERE username = ?";
            auto result = conn->execute_query(sql, username);
            
            if (result.empty()) {
                return std::nullopt;
            }

            // 解析查询结果
            UserInfo user_info;
            user_info.username = result[0][0];        // username列
            user_info.password_hash = result[0][1];   // password_hash列
            user_info.email = result[0][2];           // email列
            user_info.created_at = result[0][3];      // created_at列
            std::string is_active_str = result[0][4]; // is_active列
            user_info.is_active = (is_active_str == "1" || is_active_str == "true");

            return user_info;
        } catch (const std::exception& e) {
            ZBACKUP_LOG_ERROR("Exception getting user info: {}", e.what());
            return std::nullopt;
        }
    }

    bool UserManager::update_user(const UserInfo& user_info)
    {
        auto conn = mysql_pool_.get_connection();
        if (!conn) {
            ZBACKUP_LOG_ERROR("Failed to get database connection");
            return false;
        }

        try {
            std::string sql = "UPDATE users SET password_hash = ?, email = ?, is_active = ? WHERE username = ?";
            
            int affected_rows = conn->execute_update(sql, 
                user_info.password_hash, 
                user_info.email, 
                user_info.is_active ? 1 : 0, 
                user_info.username);
                
            if (affected_rows <= 0) {
                ZBACKUP_LOG_ERROR("Failed to update user, no rows affected");
                return false;
            }

            ZBACKUP_LOG_INFO("User '{}' updated successfully", user_info.username);
            return true;
        } catch (const std::exception& e) {
            ZBACKUP_LOG_ERROR("Exception updating user: {}", e.what());
            return false;
        }
    }

    std::string UserManager::hash_password(const std::string& password)
    {
        // 使用 std::hash 进行哈希
        std::hash<std::string> hasher;
        size_t hash_value = hasher(password);
        
        // 将哈希值转换为十六进制字符串
        std::stringstream ss;
        ss << std::hex << hash_value;
        return ss.str();
    }

    bool UserManager::verify_password(const std::string& password, const std::string& hash)
    {
        return hash_password(password) == hash;
    }
}
