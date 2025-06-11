#pragma once
#include "../interfaces/core_interfaces.h"
#include "../log/logger.h"

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
    };
}
