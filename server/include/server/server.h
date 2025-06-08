#pragma once
#include "looper.h"
#include "../compress/compress.h"
#include "../storage/storage.h"
#include "../handlers/download_handler.h"
#include "../handlers/listshow_handler.h"
#include "../handlers/upload_handler.h"
#include "../handlers/delete_handler.h"
#include "../handlers/static_handler.h"
#include <memory>
#include <functional>
#include "../../../ZHttpServer/include/http/http_server.h"

namespace zbackup
{
    class BackupServer
    {
    public:
        using ptr = std::shared_ptr<BackupServer>;
        
        BackupServer(Compress::ptr comp, Storage::ptr storage);
        void run();

    private:
        uint16_t server_port_;
        std::string server_ip_;
        std::string download_prefix_;
        std::unique_ptr<zhttp::HttpServer> server_;
        BackupLooper::ptr looper_;
    
        // 处理器
        std::shared_ptr<UploadHandler> upload_handler_;
        std::shared_ptr<ListShowHandler> list_show_handler_;
        std::shared_ptr<DownloadHandler> download_handler_;
        std::shared_ptr<DeleteHandler> delete_handler_;
        std::shared_ptr<StaticHandler> static_handler_;
    };
}
