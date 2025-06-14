#pragma once
#include "looper.h"
#include "interfaces/compress_interface.h"
#include "interfaces/storage_interface.h"
#include "interfaces/config_manager_interface.h"
#include "interfaces/server_lifecycle_interface.h"
#include "interfaces/route_registry_interface.h"
#include <memory>
#include "http/http_server.h"
#include "interfaces/backup_storage_interface.h"

namespace zbackup
{
    class BackupServer : public interfaces::IServerLifecycle
    {
    public:
        using ptr = std::shared_ptr<BackupServer>;

        BackupServer(const interfaces::ICompress::ptr &comp, const interfaces::IBackupStorage::ptr &storage);

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
        interfaces::IRouteRegistry::ptr route_registry_;
    };
}
