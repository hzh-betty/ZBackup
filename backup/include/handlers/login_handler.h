#pragma once
#include "base_handler.h"
#include "interfaces/user_manager_interface.h"

namespace zbackup
{
    // 登录处理器
    class LoginHandler final : public BaseHandler
    {
    public:
        explicit LoginHandler(interfaces::IUserManager::ptr user_manager);

        void handle_request(const zhttp::HttpRequest &req, zhttp::HttpResponse *rsp) override;

    private:
        interfaces::IUserManager::ptr user_manager_;
    };
}
