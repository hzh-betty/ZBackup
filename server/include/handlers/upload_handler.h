#pragma once
#include "base_handler.h"
#include <regex>

namespace zbackup
{
    class UploadHandler : public BaseHandler
    {
    public:
        explicit UploadHandler(Compress::ptr comp);
        void handle_request(const zhttp::HttpRequest &req, zhttp::HttpResponse *rsp) override;

    private:
        bool parse_multipart_data(const zhttp::HttpRequest &req, std::string &filename, std::string &file_content);
        bool save_file(const std::string &filename, const std::string &file_content);
    };
}
