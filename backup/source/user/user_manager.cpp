#include "user/user_manager.h"
#include <iomanip>
#include <sstream>
#include <functional>
#include "log/backup_logger.h"

namespace zbackup
{
    UserManager::UserManager(interfaces::IUserStorage::ptr user_storage)
        : user_storage_(std::move(user_storage))
    {
        if (!user_storage_) {
            ZBACKUP_LOG_ERROR("UserManager initialized with null storage");
            throw std::invalid_argument("User storage cannot be null");
        }
        ZBACKUP_LOG_INFO("UserManager initialized successfully");
    }

    bool UserManager::register_user(const std::string &username, const std::string &password,
                                    const std::string &email)
    {
        if (username.empty() || password.empty() || email.empty()) {
            ZBACKUP_LOG_WARN("Register failed: empty username, password or email");
            return false;
        }

        // 检查用户名是否已存在
        info::UserInfo existing_user;
        if (user_storage_->get_by_username(username, &existing_user)) {
            ZBACKUP_LOG_WARN("Registration failed: username '{}' already exists", username);
            return false;
        }

        // 创建新用户
        info::UserInfo new_user;
        new_user.username_ = username;
        new_user.password_hash_ = hash_password(password);
        new_user.email_ = email;
        new_user.created_at_ = std::to_string(std::time(nullptr));

        if (user_storage_->insert(new_user)) {
            ZBACKUP_LOG_INFO("User '{}' registered successfully", username);
            return true;
        } else {
            ZBACKUP_LOG_ERROR("Failed to insert user '{}'", username);
            return false;
        }
    }

    bool UserManager::validate_user(const std::string &username, const std::string &password)
    {
        if (username.empty() || password.empty()) {
            return false;
        }

        info::UserInfo user;
        if (!user_storage_->get_by_username(username, &user)) {
            ZBACKUP_LOG_WARN("Login failed: user '{}' not found", username);
            return false;
        }

        bool valid = verify_password(password, user.password_hash_);
        if (valid) {
            ZBACKUP_LOG_INFO("User '{}' logged in successfully", username);
        } else {
            ZBACKUP_LOG_WARN("Login failed: invalid password for user '{}'", username);
        }

        return valid;
    }

    std::optional<info::UserInfo> UserManager::get_user_info(const std::string &username)
    {
        info::UserInfo user;
        if (user_storage_->get_by_username(username, &user)) {
            return user;
        }
        return std::nullopt;
    }

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

    std::string UserManager::hash_password(const std::string &password)
    {
        // 使用 std::hash 进行哈希
        std::hash<std::string> hasher;
        size_t hash_value = hasher(password);

        // 将哈希值转换为十六进制字符串
        std::stringstream ss;
        ss << std::hex << hash_value;
        return ss.str();
    }

    bool UserManager::verify_password(const std::string &password, const std::string &hash)
    {
        return hash_password(password) == hash;
    }
}
