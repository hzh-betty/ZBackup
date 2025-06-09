#pragma once
#include "base_handler.h"
#include "../user/user_manager.h"

namespace zbackup
{
    // 登录处理器
    class LoginHandler : public BaseHandler
    {
    public:
        LoginHandler(Compress::ptr comp, UserManager::ptr user_manager);
        void handle_request(const zhttp::HttpRequest &req, zhttp::HttpResponse *rsp) override;

    private:
        UserManager::ptr user_manager_;
    };
}
