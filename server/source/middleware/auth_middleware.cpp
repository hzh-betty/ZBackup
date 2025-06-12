#include "../../include/middleware/auth_middleware.h"
#include "../../include/core/service_container.h"
#include "../../include/interfaces/core_interfaces.h"

namespace zbackup
{
    AuthMiddleware::AuthMiddleware()
    {
        ZBACKUP_LOG_DEBUG("AuthMiddleware initialized");
    }

    void AuthMiddleware::before(zhttp::HttpRequest &request)
    {
        std::string path = request.get_path();
        ZBACKUP_LOG_DEBUG("AuthMiddleware checking path: {}", path);

        // 公共路径，无需认证
        if (path == "/" || path == "/home.html" || path == "/home" ||
            path == "/login.html" || path == "/register.html" ||
            path == "/login" || path == "/register" ||
            path == "/api/status" || path.find("/static/") == 0)
        {
            ZBACKUP_LOG_DEBUG("Public path allowed: {}", path);
            return;
        }

        // 检查认证状态
        if (!is_authenticated(request))
        {
            ZBACKUP_LOG_WARN("Unauthorized access attempt to: {}", path);
            // 可以在这里设置重定向或其他处理
        }
    }

    void AuthMiddleware::after(zhttp::HttpResponse &response)
    {
        // 响应后处理，可以添加安全头等
        response.set_header("X-Frame-Options", "DENY");
        response.set_header("X-Content-Type-Options", "nosniff");
        response.set_header("X-XSS-Protection", "1; mode=block");
    }

    bool AuthMiddleware::is_authenticated(const zhttp::HttpRequest &req)
    {
        auto& container = core::ServiceContainer::get_instance();
        auto auth_service = container.resolve<interfaces::IAuthenticationService>();
        
        if (!auth_service) {
            ZBACKUP_LOG_ERROR("AuthenticationService not available in middleware");
            return false;
        }

        return auth_service->authenticate(req);
    }
}
