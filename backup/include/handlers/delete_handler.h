#pragma once
#include "base_handler.h"

namespace zbackup
{
    class DeleteHandler final : public BaseHandler
    {
    public:
        DeleteHandler() = default;

        void handle_request(const zhttp::HttpRequest &req, zhttp::HttpResponse *rsp) override;
    };
}
