#pragma once
#include "looper.hpp"
#include "compress.hpp"
#include "storage.hpp"
#include "handler/download_handler.hpp"
#include "handler/listshow_handler.hpp"
#include "handler/upload_handler.hpp"
#include <memory>
#include <functional>
#include "../ZHttpServer/include/http/http_server.h"

namespace zbackup
{
    class BackupServer
    {
    public:
        using ptr = std::shared_ptr<BackupServer>;
        
        BackupServer(Compress::ptr comp, Storage::ptr storage)
            : looper_(std::make_shared<BackupLooper>(comp))
        {
            // 初始化 DataManager 单例
            DataManager::getInstance(storage);
            
            Config &config = Config::getInstance();
            serverPort_ = config.getPort();
            serverIp_ = config.getIp();
            downloadPrefix_ = config.getDownloadPrefix();
            logger->debug("backup service init success");

            // 初始化ZHttpServer
            auto builder = std::make_unique<zhttp::HttpServerBuilder>();
            builder->build_port(serverPort_);
            builder->build_name("BackupServer");
            builder->build_thread_num(4);
            server_ = builder->build();

            // 创建处理器
            uploadHandler_ = std::make_shared<UploadHandler>(comp);
            listShowHandler_ = std::make_shared<ListShowHandler>(comp);
            downloadHandler_ = std::make_shared<DownloadHandler>(comp);
        }

        void run()
        {
            // 注册处理器
            server_->Post("/upload", uploadHandler_);
            server_->Get("/listshow", listShowHandler_);
            server_->Get("/", listShowHandler_);
            
            std::string downloadUrl = downloadPrefix_ + "(.*)";
            server_->add_regex_route(zhttp::HttpRequest::Method::GET, downloadUrl, downloadHandler_);

            logger->info("BackupServer starting on port {}", serverPort_);
            server_->start();
        }

    private:
        uint16_t serverPort_;
        std::string serverIp_;
        std::string downloadPrefix_;
        std::unique_ptr<zhttp::HttpServer> server_;
        BackupLooper::ptr looper_;
    
        private:
        // 处理器
        std::shared_ptr<UploadHandler> uploadHandler_;
        std::shared_ptr<ListShowHandler> listShowHandler_;
        std::shared_ptr<DownloadHandler> downloadHandler_;
    };
};