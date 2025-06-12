#include "../../include/core/session_service.h"
#include "../../../ZHttpServer/include/session/session_manager.h"

namespace zbackup::core
{
    void SessionService::create_session(const std::string &username,
                                        const zhttp::HttpRequest &req, zhttp::HttpResponse *rsp)
    {
        auto session = session_manager_.get_session(req, rsp);
        session->set_attribute("username", username);
        session->set_attribute("logged_in", "true");
        session_manager_.update_session(session);
        ZBACKUP_LOG_DEBUG("Session created for user: {}", username);
    }

    void SessionService::destroy_session(const zhttp::HttpRequest &req, zhttp::HttpResponse *rsp)
    {
        auto session = session_manager_.get_session(req, rsp);
        std::string username = session->get_attribute("username");
        session_manager_.destroy_session(session->get_session_id());
        ZBACKUP_LOG_DEBUG("Session destroyed for user: {}", username);
    }

    std::string SessionService::get_username(const zhttp::HttpRequest &req)
    {
        try
        {
            zhttp::HttpResponse dummy_response;
            auto session = session_manager_.get_session(req, &dummy_response);
            return session->get_attribute("username");
        }
        catch (const std::exception &e)
        {
            ZBACKUP_LOG_ERROR("Error getting username from session: {}", e.what());
            return "";
        }
    }

    bool SessionService::is_valid_session(const zhttp::HttpRequest &req)
    {
        try
        {
            zhttp::HttpResponse dummy_response;
            auto session = session_manager_.get_session(req, &dummy_response);

            auto logged_in = session->get_attribute("logged_in");
            auto username = session->get_attribute("username");

            return logged_in == "true" && !username.empty();
        }
        catch (const std::exception &e)
        {
            ZBACKUP_LOG_ERROR("Error checking session validity: {}", e.what());
            return false;
        }
    }
}
