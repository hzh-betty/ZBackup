#pragma once
#include "base_handler.h"
#include "interfaces/user_manager_interface.h"

namespace zbackup
{
    // 注册处理器
    class RegisterHandler final : public BaseHandler
    {
    public:
        explicit RegisterHandler(interfaces::IUserManager::ptr user_manager);

        void handle_request(const zhttp::HttpRequest &req, zhttp::HttpResponse *rsp) override;

    private:
        interfaces::IUserManager::ptr user_manager_;

        static bool validate_email(const std::string &email);
        static bool validate_password(const std::string &password);
    };
}

