#pragma once
#include "base_handler.h"
#include "../user/user_manager.h"
#include <regex>

namespace zbackup
{
    // 注册处理器
    class RegisterHandler : public BaseHandler
    {
    public:
        RegisterHandler(Compress::ptr comp, UserManager::ptr user_manager);
        void handle_request(const zhttp::HttpRequest &req, zhttp::HttpResponse *rsp) override;

    private:
        UserManager::ptr user_manager_;
        bool validate_email(const std::string& email);
        bool validate_password(const std::string& password);
    };
}
