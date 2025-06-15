#pragma once
#include "interfaces/auth_manager_interface.h"
#include "interfaces/session_manager_interface.h"
#include "interfaces/user_manager_interface.h"
#include <memory>

namespace zbackup::core
{
    class AuthenticationService : public interfaces::IAuthenticationService
    {
    public:
        AuthenticationService(interfaces::IUserManager::ptr user_manager, interfaces::ISessionManager::ptr session_manager);
        ~AuthenticationService() override = default;

        bool authenticate(const zhttp::HttpRequest &req) override;
        bool login(const std::string &username, const std::string &password,
                   const zhttp::HttpRequest &req, zhttp::HttpResponse *rsp) override;
        bool logout(const zhttp::HttpRequest &req, zhttp::HttpResponse *rsp) override;
    private:
        interfaces::IUserManager::ptr user_manager_;
        interfaces::ISessionManager::ptr session_manager_;
    };
}
