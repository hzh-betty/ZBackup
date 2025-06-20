#pragma once
#include "base_handler.h"

namespace zbackup
{
    class LogoutHandler final : public BaseHandler
    {
    public:
        LogoutHandler() = default;

        void handle_request(const zhttp::HttpRequest &req, zhttp::HttpResponse *rsp) override;
    };
}

