#pragma once
#include "base_handler.h"
#include "util/util.h"
#include "info/backup_info.h"
namespace zbackup
{
    class DownloadHandler final : public BaseHandler
    {
    public:
        DownloadHandler() = default;

        void handle_request(const zhttp::HttpRequest &req, zhttp::HttpResponse *rsp) override;

    private:
        void handle_range_request(const zhttp::HttpRequest &req, zhttp::HttpResponse *rsp,
                                 const info::BackupInfo &info, util::FileUtil &fu, int64_t file_size,
                                 const std::string &range_header);
        void handle_full_request(zhttp::HttpResponse *rsp, const info::BackupInfo &info, util::FileUtil &fu);
    };
}
