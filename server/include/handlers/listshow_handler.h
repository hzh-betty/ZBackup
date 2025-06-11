#pragma once
#include "base_handler.h"
#include <sstream>

namespace zbackup
{
    class ListShowHandler final : public DataHandler
    {
    public:
        ListShowHandler();
        void handle_request(const zhttp::HttpRequest &req, zhttp::HttpResponse *rsp) override;
    };
}
