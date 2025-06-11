#pragma once
#include "looper.h"
#include "../compress/compress.h"
#include "../storage/storage.h"
#include "../interfaces/core_interfaces.h"
#include "../core/route_registry.h"
#include <memory>
#include "../../../ZHttpServer/include/http/http_server.h"

namespace zbackup
{
    class BackupServer : public interfaces::IServerLifecycle
    {
    public:
        using ptr = std::shared_ptr<BackupServer>;

        BackupServer(const Compress::ptr &comp, const Storage::ptr &storage);

        // IServerLifecycle 接口实现
        bool initialize() override;
        void start() override;
        void stop() override;
        bool is_running() const override;

        void run();

    private:
        void initialize_server();
        void setup_routes();

        std::unique_ptr<zhttp::HttpServer> server_;
        BackupLooper::ptr looper_;
        std::atomic<bool> running_;

        // 依赖服务
        interfaces::IConfigManager::ptr config_manager_;
        core::IRouteRegistry::ptr route_registry_;
    };
}
