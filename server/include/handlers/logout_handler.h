#pragma once
#include "base_handler.h"

namespace zbackup
{
    // 登出处理器
    class LogoutHandler final : public BaseHandler
    {
    public:
        LogoutHandler();

        void handle_request(const zhttp::HttpRequest &req, zhttp::HttpResponse *rsp) override;
    };
}
