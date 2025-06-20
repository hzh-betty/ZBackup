#pragma once
#include "base_handler.h"

namespace zbackup
{
    // 登录处理器
    class LoginHandler final : public BaseHandler
    {
    public:
        LoginHandler() = default;

        void handle_request(const zhttp::HttpRequest &req, zhttp::HttpResponse *rsp) override;
    };
}