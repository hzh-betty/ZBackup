#pragma once
#include "looper.hpp"
#include "compress.hpp"
#include "httplib.h"
#include "storage.hpp"
#include <memory>
#include <functional>

namespace zbackup
{
    class BackupServer
    {
    public:
        using ptr = std::shared_ptr<BackupServer>;
        using HttpHandler_t = std::function<void(const httplib::Request &req, httplib::Response &rsp)>;
        BackupServer(Compress::ptr comp, Storage::ptr storage)
            : looper_(std::make_shared<BackupLooper>(comp, storage))
        {
            Config &config = Config::getInstance();
            serverPort_ = config.getPort();
            serverIp_ = config.getIp();
            downloadPrefix_ = config.getDownloadPrefix();
            logger->debug("backup service init success");
        }

        void run()
        {
            HttpHandler_t upload = std::bind(&BackupLooper::upload, looper_.get(), std::placeholders::_1, std::placeholders::_2);
            HttpHandler_t listshow = std::bind(&BackupLooper::listshow, looper_.get(), std::placeholders::_1, std::placeholders::_2);
            HttpHandler_t download = std::bind(&BackupLooper::download, looper_.get(), std::placeholders::_1, std::placeholders::_2);

            server_.Post("/upload", upload);
            server_.Get("/listshow", listshow);
            server_.Get("/", listshow);
            std::string downloadUrl = downloadPrefix_ + "(.*)";
            server_.Get(downloadUrl, download);
            server_.listen(serverIp_.c_str(), serverPort_);
        }

    private:
        uint16_t serverPort_;
        std::string serverIp_;
        std::string downloadPrefix_;
        httplib::Server server_;
        BackupLooper::ptr looper_;
    };
};