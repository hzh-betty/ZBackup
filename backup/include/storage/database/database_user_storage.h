#pragma once
#include "interfaces/user_storage_interface.h"
#include "info/user_info.h"
#include "db_pool/mysql_pool.h"

namespace zbackup::storage
{
    // 数据库用户存储实现类
    class DatabaseUserStorage : public zbackup::interfaces::IUserStorage
    {
    public:
        DatabaseUserStorage();

        // 实现泛型存储接口
        bool insert(const info::UserInfo &info) override;
        bool update(const info::UserInfo &info) override;
        bool get_one_by_id(const std::string &id, info::UserInfo *info) override;
        void get_all(std::vector<info::UserInfo> *arry) override;
        bool delete_one(const info::UserInfo &info) override;
        bool delete_by_id(const std::string &id) override;
        std::vector<info::UserInfo> find_by_condition(const std::function<bool(const info::UserInfo&)>& condition) override;

        // 实现用户存储特有接口
        bool get_by_username(const std::string &username, info::UserInfo *user) override;
        bool get_by_email(const std::string &email, info::UserInfo *user) override;

    private:
        bool create_table_if_not_exists();
    };
}
