#pragma once
#include "../info/user_info.h"
#include <string>
#include <memory>
#include <optional>

namespace zbackup::interfaces
{
    // 用户管理器接口
    class IUserManager
    {
    public:
        using ptr = std::shared_ptr<IUserManager>;
        virtual ~IUserManager() = default;

        // 用户注册
        virtual bool register_user(const std::string &username, const std::string &password, 
                                  const std::string &email) = 0;

        // 用户登录验证
        virtual bool validate_user(const std::string &username, const std::string &password) = 0;

        // 获取用户信息
        virtual std::optional<info::UserInfo> get_user_info(const std::string &username) = 0;

        // 更新用户信息
        virtual bool update_user(const info::UserInfo &user_info) = 0;

        // 密码哈希
        virtual std::string hash_password(const std::string &password) = 0;

        // 验证密码
        virtual bool verify_password(const std::string &password, const std::string &hash) = 0;
    };
}
