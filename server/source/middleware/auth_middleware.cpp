#include "../../include/middleware/auth_middleware.h"
#include <nlohmann/json.hpp>

namespace zbackup
{
    AuthMiddleware::AuthMiddleware() {}

    void AuthMiddleware::before(zhttp::HttpRequest &req)
    {
        if (!is_authenticated(req)) {
            ZBACKUP_LOG_WARN("Unauthorized access attempt to {}", req.get_path());
            
            // 设置401响应状态，让框架处理
            throw std::runtime_error("Authentication required");
        }
    }

    void AuthMiddleware::after(zhttp::HttpResponse &rsp)
    {
        // 响应后处理 - 目前不需要特殊处理
        // 可以在这里添加响应头或日志记录
    }

    bool AuthMiddleware::is_authenticated(const zhttp::HttpRequest &req)
    {
        try {
            zhttp::HttpResponse dummy_response;
            auto session = zhttp::zsession::SessionManager::get_instance().get_session(req, &dummy_response);
            
            auto logged_in = session->get_attribute("logged_in");
            auto username = session->get_attribute("username");
            
            return logged_in == "true" && !username.empty();
        } catch (const std::exception& e) {
            ZBACKUP_LOG_ERROR("Error checking authentication: {}", e.what());
            return false;
        }
    }
}
