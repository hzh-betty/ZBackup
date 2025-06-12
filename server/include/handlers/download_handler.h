#pragma once
#include "base_handler.h"

namespace zbackup
{
    class DownloadHandler final : public CompressHandler
    {
    public:
        explicit DownloadHandler(interfaces::ICompress::ptr comp);

        void handle_request(const zhttp::HttpRequest &req, zhttp::HttpResponse *rsp) override;

    private:
        void handle_range_request(const zhttp::HttpRequest &req, zhttp::HttpResponse *rsp,
                                         const BackupInfo &info, FileUtil &fu, int64_t file_size,
                                         const std::string &range_header);

        void handle_full_request(zhttp::HttpResponse *rsp, const BackupInfo &info, FileUtil &fu);
    };
}
