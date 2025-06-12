#include "../../include/handlers/logout_handler.h"
#include "../../include/core/service_container.h"
#include "../../include/interfaces/auth_manager_interface.h"
#include "../../include/interfaces/session_manager_interface.h"
#include <nlohmann/json.hpp>
#include <utility>

namespace zbackup
{
    LogoutHandler::LogoutHandler() = default;

    void LogoutHandler::handle_request(const zhttp::HttpRequest &req, zhttp::HttpResponse *rsp)
    {
        auto &container = core::ServiceContainer::get_instance();
        auto auth_service = container.resolve<interfaces::IAuthenticationService>();

        if (!auth_service)
        {
            ZBACKUP_LOG_ERROR("AuthenticationService not available");
            rsp->set_status_code(zhttp::HttpResponse::StatusCode::InternalServerError);
            rsp->set_status_message("Internal Server Error");
            rsp->set_content_type("application/json");
            rsp->set_body(R"({"error": "Authentication service unavailable"})");
            return;
        }

        // 获取用户名用于日志记录
        auto session_service = container.resolve<interfaces::ISessionManager>();
        std::string username = session_service ? session_service->get_username(req) : "unknown";

        // 使用AuthenticationService进行退出登录
        if (auth_service->logout(req, rsp))
        {
            nlohmann::json resp_json;
            resp_json["success"] = true;
            resp_json["message"] = "Logout successful";

            std::string response_body;
            JsonUtil::serialize(resp_json, &response_body);

            rsp->set_status_code(zhttp::HttpResponse::StatusCode::OK);
            rsp->set_status_message("OK");
            rsp->set_content_type("application/json");
            rsp->set_body(response_body);

            ZBACKUP_LOG_INFO("User '{}' logged out", username.empty() ? "unknown" : username);
        }
        else
        {
            nlohmann::json resp_json;
            resp_json["success"] = false;
            resp_json["error"] = "Logout failed";

            std::string response_body;
            JsonUtil::serialize(resp_json, &response_body);

            rsp->set_status_code(zhttp::HttpResponse::StatusCode::InternalServerError);
            rsp->set_status_message("Internal Server Error");
            rsp->set_content_type("application/json");
            rsp->set_body(response_body);

            ZBACKUP_LOG_ERROR("Logout failed for user '{}'", username);
        }
    }
}
