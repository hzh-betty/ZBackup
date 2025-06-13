#include "../../include/middleware/auth_middleware.h"
#include "../../include/core/service_container.h"
#include "../../include/interfaces/session_manager_interface.h"

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
        if (is_public_path(path))
        {
            ZBACKUP_LOG_DEBUG("Public path allowed: {}", path);
            return;
        }

        // 检查认证状态
        if (!is_authenticated(request))
        {
            ZBACKUP_LOG_WARN("Unauthorized access attempt to: {}, redirecting to home", path);

            // 直接抛出重定向响应
            zhttp::HttpResponse redirect_response;
            redirect_response.set_status_code(zhttp::HttpResponse::StatusCode::Found);
            redirect_response.set_status_message("Found");
            redirect_response.set_header("Location", "/home.html");
            redirect_response.set_content_type("text/html");
            redirect_response.set_body("<html><body>Redirecting to home page...</body></html>");

            throw redirect_response;
        }
    }

    void AuthMiddleware::after(zhttp::HttpResponse &response)
    {
        // 响应后处理，可以添加安全头等
        response.set_header("X-Frame-Options", "DENY");
        response.set_header("X-Content-Type-Options", "nosniff");
        response.set_header("X-XSS-Protection", "1; mode=block");
    }

    bool AuthMiddleware::is_public_path(const std::string &path)
    {
        // 公共路径列表
        const std::vector<std::string> public_paths = {
            "/", "/home.html", "/home",
            "/login.html", "/register.html",
            "/login", "/register",
            "/api/status"};

        // 检查是否为公共路径
        for (const auto &public_path : public_paths)
        {
            if (path == public_path)
            {
                return true;
            }
        }

        // 检查是否为静态资源
        if (path.find("/static/") == 0 ||
            path.find("/css/") == 0 ||
            path.find("/js/") == 0 ||
            path.find("/images/") == 0)
        {
            return true;
        }

        return false;
    }

    bool AuthMiddleware::is_authenticated(const zhttp::HttpRequest &req)
    {
        auto &container = core::ServiceContainer::get_instance();
        auto session_service = container.resolve<interfaces::ISessionManager>();

        if (!session_service)
        {
            ZBACKUP_LOG_ERROR("SessionService not available in middleware");
            return false;
        }

        return session_service->is_valid_session(req);
    }
}
