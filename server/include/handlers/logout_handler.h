#pragma once
#include "base_handler.h"

namespace zbackup
{
    // 登出处理器
    class LogoutHandler : public BaseHandler
    {
    public:
        explicit LogoutHandler(Compress::ptr comp);
        void handle_request(const zhttp::HttpRequest &req, zhttp::HttpResponse *rsp) override;
    };
}
