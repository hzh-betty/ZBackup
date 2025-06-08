#pragma once
#include "base_handler.h"
#include <sstream>

namespace zbackup
{
    class ListShowHandler : public BaseHandler
    {
    public:
        explicit ListShowHandler(Compress::ptr comp);
        void handle_request(const zhttp::HttpRequest &req, zhttp::HttpResponse *rsp) override;
    };
}
