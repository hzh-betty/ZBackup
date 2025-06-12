#pragma once
#include "../interfaces/session_manager_interface.h"
#include "../log/logger.h"
#include "../../../ZHttpServer/include/session/session_manager.h"

namespace zbackup::core
{
    class SessionService : public interfaces::ISessionManager
    {
    public:
        SessionService() = default;
        ~SessionService() override = default;

        void create_session(const std::string& username,
                          const zhttp::HttpRequest& req, zhttp::HttpResponse* rsp) override;
        void destroy_session(const zhttp::HttpRequest& req, zhttp::HttpResponse* rsp) override;
        std::string get_username(const zhttp::HttpRequest& req) override;
        bool is_valid_session(const zhttp::HttpRequest& req) override;
    protected:
        zhttp::zsession::SessionManager& session_manager_ = zhttp::zsession::SessionManager::get_instance();
    };
}
