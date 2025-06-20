#pragma once
#include "base_handler.h"

namespace zbackup
{
    class ListShowHandler final : public BaseHandler
    {
    public:
        ListShowHandler() = default;

        void handle_request(const zhttp::HttpRequest &req, zhttp::HttpResponse *rsp) override;
    };
}
