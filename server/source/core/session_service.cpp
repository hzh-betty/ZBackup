#include "../../include/core/session_service.h"
#include "../../../ZHttpServer/include/session/session_manager.h"

namespace zbackup::core
{
    void SessionService::create_session(const std::string& username,
                                       const zhttp::HttpRequest& req, zhttp::HttpResponse* rsp)
    {
        auto session = zhttp::zsession::SessionManager::get_instance().get_session(req, rsp);
        session->set_attribute("username", username);
        session->set_attribute("logged_in", "true");
        zhttp::zsession::SessionManager::get_instance().update_session(session);
    }

    void SessionService::destroy_session(const zhttp::HttpRequest& req, zhttp::HttpResponse* rsp)
    {
        auto session = zhttp::zsession::SessionManager::get_instance().get_session(req, rsp);
        session->remove_attribute("username");
        session->remove_attribute("logged_in");
        zhttp::zsession::SessionManager::get_instance().update_session(session);
    }

    std::string SessionService::get_username(const zhttp::HttpRequest& req)
    {
        try {
            zhttp::HttpResponse dummy_response;
            auto session = zhttp::zsession::SessionManager::get_instance().get_session(req, &dummy_response);
            return session->get_attribute("username");
        }
        catch (const std::exception& e) {
            ZBACKUP_LOG_ERROR("Error getting username from session: {}", e.what());
            return "";
        }
    }

    bool SessionService::is_valid_session(const zhttp::HttpRequest& req)
    {
        try {
            zhttp::HttpResponse dummy_response;
            auto session = zhttp::zsession::SessionManager::get_instance().get_session(req, &dummy_response);
            
            auto logged_in = session->get_attribute("logged_in");
            auto username = session->get_attribute("username");
            
            return logged_in == "true" && !username.empty();
        }
        catch (const std::exception& e) {
            ZBACKUP_LOG_ERROR("Error checking session validity: {}", e.what());
            return false;
        }
    }
}
