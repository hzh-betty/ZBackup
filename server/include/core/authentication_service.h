#pragma once
#include "../interfaces/core_interfaces.h"
#include "../log/logger.h"
#include <memory>

namespace zbackup
{
    class UserManager;
}

namespace zbackup::core
{
    class AuthenticationService : public interfaces::IAuthenticationService
    {
    public:
        AuthenticationService();
        ~AuthenticationService() override = default;

        bool authenticate(const zhttp::HttpRequest& req) override;
        bool login(const std::string& username, const std::string& password,
                  const zhttp::HttpRequest& req, zhttp::HttpResponse* rsp) override;
        bool logout(const zhttp::HttpRequest& req, zhttp::HttpResponse* rsp) override;

    private:
       bool ensure_dependencies();  // 延迟初始化依赖
       private:
        std::shared_ptr<UserManager> user_manager_;
        interfaces::ISessionManager::ptr session_manager_;
    };
}
