#pragma once
#include "../interfaces/user_manager_interface.h"
#include "../interfaces/user_storage_interface.h"
#include "../log/logger.h"
#include <memory>

namespace zbackup
{
    // 用户管理类
    class UserManager : public interfaces::IUserManager
    {
    public:
        explicit UserManager(interfaces::IUserStorage::ptr user_storage);
        ~UserManager() override = default;

        // 禁用拷贝和赋值
        UserManager(const UserManager &) = delete;
        UserManager &operator=(const UserManager &) = delete;

        // IUserManager 接口实现
        bool register_user(const std::string &username, const std::string &password,
                          const std::string &email) override;
        bool validate_user(const std::string &username, const std::string &password) override;
        std::optional<info::UserInfo> get_user_info(const std::string &username) override;
        bool update_user(const info::UserInfo &user_info) override;
        std::string hash_password(const std::string &password) override;
        bool verify_password(const std::string &password, const std::string &hash) override;

    private:
        interfaces::IUserStorage::ptr user_storage_;
    };
}
