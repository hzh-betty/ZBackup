#pragma once
#include "storage_interface.h"

// 前向声明
namespace zbackup::info
{
    class UserInfo;
}

namespace zbackup::interfaces
{
    // 用户信息专用存储接口  
    class IUserStorage : public IStorage<info::UserInfo>
    {
    public:
        using ptr = std::shared_ptr<IUserStorage>;
        
        // 用户存储特有接口
        virtual bool get_by_username(const std::string &username, info::UserInfo *user) = 0;
        virtual bool get_by_email(const std::string &email, info::UserInfo *user) = 0;
    };
}
