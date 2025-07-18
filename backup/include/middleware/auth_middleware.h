#pragma once
#include "middleware/middleware.h"
#include "util/util.h"
#include "session/session_manager.h"

namespace zbackup
{
    // 认证中间件
    class AuthMiddleware final : public zhttp::zmiddleware::Middleware
    {
    public:
        AuthMiddleware();

        // 请求前处理
        void before(zhttp::HttpRequest &request) override;

        // 响应后处理
        void after(zhttp::HttpResponse &response) override;

    private:
        bool is_authenticated(const zhttp::HttpRequest &req);
        bool is_public_path(const std::string& path);
    };
}
