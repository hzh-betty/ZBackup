#include "../../include/handlers/logout_handler.h"
#include "../../ZHttpServer/include/session/session_manager.h"
#include <nlohmann/json.hpp>
#include <utility>

namespace zbackup
{
    LogoutHandler::LogoutHandler(Compress::ptr comp) : BaseHandler(std::move(comp)) {}

    void LogoutHandler::handle_request(const zhttp::HttpRequest &req, zhttp::HttpResponse *rsp)
    {
        auto session = zhttp::zsession::SessionManager::get_instance().get_session(req, rsp);

        std::string username = session->get_attribute("username");

        // 销毁会话
        zhttp::zsession::SessionManager::get_instance().destroy_session(session->get_session_id());

        nlohmann::json resp_json;
        resp_json["success"] = true;
        resp_json["message"] = "Logout successful";

        std::string response_body;
        JsonUtil::serialize(resp_json, &response_body);

        rsp->set_status_code(zhttp::HttpResponse::StatusCode::OK);
        rsp->set_status_message("OK");
        rsp->set_content_type("application/json");
        rsp->set_body(response_body);

        ZBACKUP_LOG_INFO("User '{}' logged out", username);
    }
}
