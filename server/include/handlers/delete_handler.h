#pragma once
#include "base_handler.h"

namespace zbackup
{
    class DeleteHandler : public BaseHandler
    {
    public:
        explicit DeleteHandler(Compress::ptr comp);
        void handle_request(const zhttp::HttpRequest &req, zhttp::HttpResponse *rsp) override;
    };
}
