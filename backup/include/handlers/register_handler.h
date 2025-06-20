#pragma once
#include "base_handler.h"

namespace zbackup
{
    // 注册处理器
    class RegisterHandler final : public BaseHandler
    {
    public:
        RegisterHandler() = default;

        void handle_request(const zhttp::HttpRequest &req, zhttp::HttpResponse *rsp) override;

    private:
        static bool validate_email(const std::string &email);
        static bool validate_password(const std::string &password);
    };
}
