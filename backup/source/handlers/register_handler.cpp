#include "handlers/register_handler.h"
#include "util/util.h"
#include <nlohmann/json.hpp>
#include <utility>
#include "log/backup_logger.h"
#include "interfaces/user_manager_interface.h"
#include "core/service_container.h"
#include <regex>
namespace zbackup
{
    void RegisterHandler::handle_request(const zhttp::HttpRequest &req, zhttp::HttpResponse *rsp)
    {
        ZBACKUP_LOG_DEBUG("Register request received");

        auto& container = core::ServiceContainer::get_instance();
        auto user_manager = container.resolve<interfaces::IUserManager>();

        if (!user_manager)
        {
            ZBACKUP_LOG_ERROR("UserManager not available");
            rsp->set_status_code(zhttp::HttpResponse::StatusCode::InternalServerError);
            rsp->set_status_message("Internal Server Error");
            rsp->set_content_type("application/json");
            rsp->set_body(R"({"error": "User service unavailable"})");
            return;
        }

        std::string content_type = req.get_header("Content-Type");

        if (content_type.find("application/json") == std::string::npos)
        {
            rsp->set_status_code(zhttp::HttpResponse::StatusCode::BadRequest);
            rsp->set_status_message("Bad Request");
            rsp->set_content_type("application/json");
            rsp->set_body(R"({"error": "Content-Type must be application/json"})");
            return;
        }

        nlohmann::json req_json;
        if (!util::JsonUtil::deserialize(&req_json, req.get_content()))
        {
            rsp->set_status_code(zhttp::HttpResponse::StatusCode::BadRequest);
            rsp->set_status_message("Bad Request");
            rsp->set_content_type("application/json");
            rsp->set_body(R"({"error": "Invalid JSON format"})");
            return;
        }

        if (!req_json.contains("username") || !req_json.contains("password") || !req_json.contains("email"))
        {
            rsp->set_status_code(zhttp::HttpResponse::StatusCode::BadRequest);
            rsp->set_status_message("Bad Request");
            rsp->set_content_type("application/json");
            rsp->set_body(R"({"error": "Username, password and email are required"})");
            return;
        }

        std::string username = req_json["username"];
        std::string password = req_json["password"];
        std::string email = req_json["email"];

        // 验证输入
        if (username.length() < 3 || username.length() > 50)
        {
            rsp->set_status_code(zhttp::HttpResponse::StatusCode::BadRequest);
            rsp->set_status_message("Bad Request");
            rsp->set_content_type("application/json");
            rsp->set_body(R"({"error": "Username must be 3-50 characters long"})");
            return;
        }

        if (!validate_password(password))
        {
            rsp->set_status_code(zhttp::HttpResponse::StatusCode::BadRequest);
            rsp->set_status_message("Bad Request");
            rsp->set_content_type("application/json");
            rsp->set_body(R"({"error": "Password must be at least 6 characters long"})");
            return;
        }

        if (!validate_email(email))
        {
            rsp->set_status_code(zhttp::HttpResponse::StatusCode::BadRequest);
            rsp->set_status_message("Bad Request");
            rsp->set_content_type("application/json");
            rsp->set_body(R"({"error": "Invalid email format"})");
            return;
        }

        // 注册用户
        if (user_manager->register_user(username, password, email))
        {
            nlohmann::json resp_json;
            resp_json["success"] = true;
            resp_json["message"] = "Registration successful";

            std::string response_body;
            util::JsonUtil::serialize(resp_json, &response_body);

            rsp->set_status_code(zhttp::HttpResponse::StatusCode::Created);
            rsp->set_status_message("Created");
            rsp->set_content_type("application/json");
            rsp->set_body(response_body);

            ZBACKUP_LOG_INFO("User '{}' registered successfully", username);
        }
        else
        {
            nlohmann::json resp_json;
            resp_json["success"] = false;
            resp_json["error"] = "Registration failed, username may already exist";

            std::string response_body;
            util::JsonUtil::serialize(resp_json, &response_body);

            rsp->set_status_code(zhttp::HttpResponse::StatusCode::Conflict);
            rsp->set_status_message("Conflict");
            rsp->set_content_type("application/json");
            rsp->set_body(response_body);
        }
    }

    bool RegisterHandler::validate_email(const std::string &email)
    {
        const std::regex email_pattern(R"(^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}$)");
        return std::regex_match(email, email_pattern);
    }

    bool RegisterHandler::validate_password(const std::string &password)
    {
        return password.length() >= 6;
    }
}
