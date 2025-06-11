#pragma once
#include "base_handler.h"
#include "../compress/compress.h"

namespace zbackup
{
    // 登出处理器
    class LogoutHandler final : public BaseHandler
    {
    public:
        explicit LogoutHandler(Compress::ptr comp);

        void handle_request(const zhttp::HttpRequest &req, zhttp::HttpResponse *rsp) override;
    };
}
