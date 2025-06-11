#pragma once
#include "../util/util.h"
#include "../log/logger.h"
#include "../../../ZHttpServer/include/db_pool/mysql_pool.h"
#include <string>
#include <memory>
#include <optional>
#include <utility>

namespace zbackup
{
    // 用户信息结构体
    struct UserInfo
    {
        std::string username;
        std::string password_hash;
        std::string email;
        std::string created_at;
        bool is_active = true;

        UserInfo() = default;

        UserInfo(std::string user, const std::string &pass, const std::string &mail)
            : username(std::move(user)), password_hash(pass), email(mail)
        {
        }
    };

    // 用户管理类
    class UserManager
    {
    public:
        using ptr = std::shared_ptr<UserManager>;

        UserManager();

        ~UserManager() = default;

        // 初始化用户表
        bool init_user_table() const;

        // 用户注册
        bool register_user(const std::string &username, const std::string &password, const std::string &email) const;

        // 用户登录验证
        bool validate_user(const std::string &username, const std::string &password) const;

        // 获取用户信息
        std::optional<UserInfo> get_user_info(const std::string &username) const;

        // 更新用户信息
        bool update_user(const UserInfo &user_info) const;

        // 密码哈希
        static std::string hash_password(const std::string &password);

        // 验证密码
        static bool verify_password(const std::string &password, const std::string &hash);

    private:
        zhttp::zdb::MysqlConnectionPool &mysql_pool_;
    };
}
