#pragma once
#include "looper.hpp"
#include "compress.hpp"
#include "storage.hpp"
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
            : looper_(std::make_shared<BackupLooper>(comp, storage))
        {
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
        }

        void run()
        {
            // 适配器，将ZHttpServer的回调转为原有逻辑
            auto upload = [this](const zhttp::HttpRequest &req, zhttp::HttpResponse *rsp) {

                looper_->upload(req, *rsp);
            };
            auto listshow = [this](const zhttp::HttpRequest &req, zhttp::HttpResponse *rsp) {
                looper_->listshow(req, *rsp);
            };
            auto download = [this](const zhttp::HttpRequest &req, zhttp::HttpResponse *rsp) {
                looper_->download(req, *rsp);
            };

            server_->Post("/upload", upload);
            server_->Get("/listshow", listshow);
            server_->Get("/", listshow);
            std::string downloadUrl = downloadPrefix_ + "(.*)";
            server_->Get(downloadUrl, download);

            logger->info("BackupServer starting on port {}", serverPort_);
            server_->start();
        }

    private:
        uint16_t serverPort_;
        std::string serverIp_;
        std::string downloadPrefix_;
        std::unique_ptr<zhttp::HttpServer> server_;
        BackupLooper::ptr looper_;
    };
};