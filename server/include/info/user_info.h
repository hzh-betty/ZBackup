#pragma once
#include "../interfaces/base_info_interface.h"
#include <utility>

namespace zbackup::info
{
    // 用户信息类
    class UserInfo : public interfaces::IBaseInfo
    {
    public:
        // 构造函数
        UserInfo() = default;
        UserInfo(std::string user, const std::string &pass, const std::string &mail)
            : username_(std::move(user)), password_hash_(pass), email_(mail) {}

        // 实现基础接口
        std::string get_id() const override { return username_; }
        void set_id(const std::string& id) override { username_ = id; }
        std::string serialize() const override;
        bool deserialize(const std::string& data) override;
        interfaces::IBaseInfo::ptr clone() const override;
        
        // 公共成员变量
        std::string username_;
        std::string password_hash_;
        std::string email_;
        std::string created_at_;
        bool is_active_ = true;
    };
}
