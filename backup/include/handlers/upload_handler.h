#pragma once
#include "base_handler.h"

namespace zbackup
{
    class UploadHandler final : public BaseHandler
    {
    public:
        UploadHandler() = default;

        void handle_request(const zhttp::HttpRequest &req, zhttp::HttpResponse *rsp) override;

    private:
        static bool parse_multipart_data(const zhttp::HttpRequest &req, std::string &filename, std::string &file_content);
        bool save_file(const std::string &filename, const std::string &file_content) const;
    };
}

