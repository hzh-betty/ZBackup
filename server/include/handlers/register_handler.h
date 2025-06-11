#pragma once
#include "base_handler.h"
#include "../user/user_manager.h"
#include <regex>

namespace zbackup
{
    // 注册处理器
    class RegisterHandler final : public BaseHandler
    {
    public:
        explicit RegisterHandler(UserManager::ptr user_manager);

        void handle_request(const zhttp::HttpRequest &req, zhttp::HttpResponse *rsp) override;

    private:
        UserManager::ptr user_manager_;

        static bool validate_email(const std::string &email);

        static bool validate_password(const std::string &password);
    };
}
