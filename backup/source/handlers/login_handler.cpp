#include "handlers/login_handler.h"
#include "core/service_container.h"
#include "interfaces/auth_manager_interface.h"
#include "interfaces/user_manager_interface.h"
#include "util/util.h"
#include <nlohmann/json.hpp>
#include "log/backup_logger.h"

namespace zbackup
{
    void LoginHandler::handle_request(const zhttp::HttpRequest &req, zhttp::HttpResponse *rsp)
    {
        ZBACKUP_LOG_DEBUG("Login request received from {}", req.get_header("X-Real-IP"));

        std::string content_type = req.get_header("Content-Type");

        if (content_type.find("application/json") == std::string::npos)
        {
            ZBACKUP_LOG_WARN("Login request with invalid content type: {}", content_type);
            rsp->set_status_code(zhttp::HttpResponse::StatusCode::BadRequest);
            rsp->set_status_message("Bad Request");
            rsp->set_content_type("application/json");
            rsp->set_body(R"({"error": "Content-Type must be application/json"})");
            return;
        }

        nlohmann::json req_json;
        if (!util::JsonUtil::deserialize(&req_json, req.get_content()))
        {
            ZBACKUP_LOG_WARN("Failed to parse login request JSON");
            rsp->set_status_code(zhttp::HttpResponse::StatusCode::BadRequest);
            rsp->set_status_message("Bad Request");
            rsp->set_content_type("application/json");
            rsp->set_body(R"({"error": "Invalid JSON format"})");
            return;
        }

        if (!req_json.contains("username") || !req_json.contains("password"))
        {
            ZBACKUP_LOG_WARN("Login request missing username or password");
            rsp->set_status_code(zhttp::HttpResponse::StatusCode::BadRequest);
            rsp->set_status_message("Bad Request");
            rsp->set_content_type("application/json");
            rsp->set_body(R"({"error": "Username and password are required"})");
            return;
        }

        std::string username = req_json["username"];
        std::string password = req_json["password"];

        // 使用AuthenticationService进行登录
        auto& container = core::ServiceContainer::get_instance();
        auto auth_service = container.resolve<interfaces::IAuthenticationService>();
        
        if (!auth_service) {
            ZBACKUP_LOG_ERROR("AuthenticationService not available");
            rsp->set_status_code(zhttp::HttpResponse::StatusCode::InternalServerError);
            rsp->set_status_message("Internal Server Error");
            rsp->set_content_type("application/json");
            rsp->set_body(R"({"error": "Authentication service unavailable"})");
            return;
        }

        if (auth_service->login(username, password, req, rsp))
        {
            nlohmann::json resp_json;
            resp_json["success"] = true;
            resp_json["message"] = "Login successful";
            resp_json["username"] = username;
            resp_json["redirect"] = "/index.html";

            std::string response_body;
            util::JsonUtil::serialize(resp_json, &response_body);

            rsp->set_status_code(zhttp::HttpResponse::StatusCode::OK);
            rsp->set_status_message("OK");
            rsp->set_content_type("application/json");
            rsp->set_body(response_body);

            ZBACKUP_LOG_INFO("User '{}' logged in successfully", username);
        }
        else
        {
            nlohmann::json resp_json;
            resp_json["success"] = false;
            resp_json["error"] = "Invalid username or password";

            std::string response_body;
            util::JsonUtil::serialize(resp_json, &response_body);

            rsp->set_status_code(zhttp::HttpResponse::StatusCode::Unauthorized);
            rsp->set_status_message("Unauthorized");
            rsp->set_content_type("application/json");
            rsp->set_body(response_body);

            ZBACKUP_LOG_WARN("Failed login attempt for user '{}'", username);
        }
    }
}
