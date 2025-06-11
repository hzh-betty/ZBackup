#pragma once
#include "looper.h"
#include "../compress/compress.h"
#include "../storage/storage.h"
#include "../handlers/download_handler.h"
#include "../handlers/listshow_handler.h"
#include "../handlers/upload_handler.h"
#include "../handlers/delete_handler.h"
#include "../handlers/static_handler.h"
#include "../handlers/login_handler.h"
#include "../handlers/register_handler.h"
#include "../handlers/logout_handler.h"
#include "../middleware/auth_middleware.h"
#include "../user/user_manager.h"
#include <memory>
#include <functional>
#include "../../../ZHttpServer/include/http/http_server.h"

namespace zbackup
{
    class BackupServer
    {
    public:
        using ptr = std::shared_ptr<BackupServer>;
        using HandlerPtr = zhttp::zrouter::Router::HandlerPtr;

        BackupServer(const Compress::ptr &comp, const Storage::ptr &storage);

        void run();

    private:
        uint16_t server_port_;
        std::string server_ip_;
        std::string download_prefix_;
        std::unique_ptr<zhttp::HttpServer> server_;
        BackupLooper::ptr looper_;

        // 用户管理
        UserManager::ptr user_manager_;

        // 处理器
        HandlerPtr upload_handler_;
        HandlerPtr list_show_handler_;
        HandlerPtr download_handler_;
        HandlerPtr delete_handler_;
        HandlerPtr static_handler_;

        // 认证处理器
        HandlerPtr login_handler_;
        HandlerPtr register_handler_;
        HandlerPtr logout_handler_;
    };
}
