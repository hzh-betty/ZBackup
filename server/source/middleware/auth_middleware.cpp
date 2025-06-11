#include "../../include/middleware/auth_middleware.h"
#include <nlohmann/json.hpp>

namespace zbackup
{
    AuthMiddleware::AuthMiddleware() = default;

    void AuthMiddleware::before(zhttp::HttpRequest &req)
    {
        // 允许所有请求通过，不进行认证检查
        ZBACKUP_LOG_DEBUG("AuthMiddleware: allowing access to {}", req.get_path());
    }

    void AuthMiddleware::after(zhttp::HttpResponse &rsp)
    {
        // 响应后处理 - 目前不需要特殊处理
        // 可以在这里添加响应头或日志记录
    }

    bool AuthMiddleware::is_authenticated(const zhttp::HttpRequest &req)
    {
        try
        {
            zhttp::HttpResponse dummy_response;
            auto session = zhttp::zsession::SessionManager::get_instance().get_session(req, &dummy_response);

            auto logged_in = session->get_attribute("logged_in");
            auto username = session->get_attribute("username");

            return logged_in == "true" && !username.empty();
        }
        catch (const std::exception &e)
        {
            ZBACKUP_LOG_ERROR("Error checking authentication: {}", e.what());
            return false;
        }
    }
}
