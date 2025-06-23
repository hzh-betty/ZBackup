/**
 * @file user_manager.cpp
 * @brief 用户管理器实现，处理用户注册、验证、密码哈希等功能
 */

#include "user/user_manager.h"
#include <iomanip>
#include <sstream>
#include <functional>
#include "log/backup_logger.h"

namespace zbackup
{
    /**
     * @brief 构造函数，初始化用户存储
     * @param user_storage 用户存储接口实现
     */
    UserManager::UserManager(interfaces::IUserStorage::ptr user_storage)
        : user_storage_(std::move(user_storage))
    {
        if (!user_storage_) {
            ZBACKUP_LOG_ERROR("UserManager initialized with null storage");
            throw std::invalid_argument("User storage cannot be null");
        }
        ZBACKUP_LOG_INFO("UserManager initialized successfully");
    }

    /**
     * @brief 用户注册
     * @param username 用户名
     * @param password 密码
     * @param email 邮箱
     * @return 注册成功返回true，失败返回false
     */
    bool UserManager::register_user(const std::string &username, const std::string &password,
                                    const std::string &email)
    {
        // 1. 验证输入参数
        if (username.empty() || password.empty() || email.empty()) {
            ZBACKUP_LOG_WARN("Register failed: empty username, password or email");
            return false;
        }

        // 2. 检查用户名是否已存在
        info::UserInfo existing_user;
        if (user_storage_->get_by_username(username, &existing_user)) {
            ZBACKUP_LOG_WARN("Registration failed: username '{}' already exists", username);
            return false;
        }

        // 3. 创建新用户信息
        info::UserInfo new_user;
        new_user.username_ = username;
        new_user.password_hash_ = hash_password(password);
        new_user.email_ = email;
        
        // 4. 生成当前时间的格式化字符串（修复时间戳问题）
        auto now = std::time(nullptr);
        auto* tm_info = std::localtime(&now);
        std::ostringstream oss;
        oss << std::put_time(tm_info, "%Y-%m-%d %H:%M:%S");
        new_user.created_at_ = oss.str();

        // 5. 插入用户到存储
        if (user_storage_->insert(new_user)) {
            ZBACKUP_LOG_INFO("User '{}' registered successfully", username);
            return true;
        } else {
            ZBACKUP_LOG_ERROR("Failed to insert user '{}'", username);
            return false;
        }
    }

    /**
     * @brief 验证用户登录信息
     * @param username 用户名
     * @param password 密码
     * @return 验证成功返回true，失败返回false
     */
    bool UserManager::validate_user(const std::string &username, const std::string &password)
    {
        // 1. 检查输入参数
        if (username.empty() || password.empty()) {
            return false;
        }

        // 2. 查找用户
        info::UserInfo user;
        if (!user_storage_->get_by_username(username, &user)) {
            ZBACKUP_LOG_WARN("Login failed: user '{}' not found", username);
            return false;
        }

        // 3. 验证密码
        bool valid = verify_password(password, user.password_hash_);
        if (valid) {
            ZBACKUP_LOG_INFO("User '{}' validated successfully", username);
        } else {
            ZBACKUP_LOG_WARN("Login failed: invalid password for user '{}'", username);
        }

        return valid;
    }

    /**
     * @brief 获取用户信息
     * @param username 用户名
     * @return 用户信息的可选对象
     */
    std::optional<info::UserInfo> UserManager::get_user_info(const std::string &username)
    {
        info::UserInfo user;
        if (user_storage_->get_by_username(username, &user)) {
            return user;
        }
        return std::nullopt;
    }

    /**
     * @brief 更新用户信息
     * @param user_info 用户信息
     * @return 更新成功返回true，失败返回false
     */
    bool UserManager::update_user(const info::UserInfo &user_info)
    {
        if (user_storage_->update(user_info)) {
            ZBACKUP_LOG_INFO("User '{}' updated successfully", user_info.username_);
            return true;
        } else {
            ZBACKUP_LOG_ERROR("Failed to update user '{}'", user_info.username_);
            return false;
        }
    }

    /**
     * @brief 密码哈希处理
     * @param password 原始密码
     * @return 哈希后的密码字符串
     */
    std::string UserManager::hash_password(const std::string &password)
    {
        // 使用 std::hash 进行哈希（生产环境建议使用更安全的哈希算法）
        std::hash<std::string> hasher;
        size_t hash_value = hasher(password);

        // 将哈希值转换为十六进制字符串
        std::stringstream ss;
        ss << std::hex << hash_value;
        return ss.str();
    }

    /**
     * @brief 验证密码
     * @param password 原始密码
     * @param hash 存储的哈希值
     * @return 验证成功返回true，失败返回false
     */
    bool UserManager::verify_password(const std::string &password, const std::string &hash)
    {
        return hash_password(password) == hash;
    }
}
