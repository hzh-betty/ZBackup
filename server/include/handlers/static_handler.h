#pragma once
#include "base_handler.h"
#include <unordered_map>

namespace zbackup
{
    class StaticHandler final : public BaseHandler
    {
    public:
        explicit StaticHandler(Compress::ptr comp);

        void handle_request(const zhttp::HttpRequest &req, zhttp::HttpResponse *rsp) override;

    private:
        std::string get_mime_type(const std::string &extension);

        std::unordered_map<std::string, std::string> mime_types_;
    };
}
