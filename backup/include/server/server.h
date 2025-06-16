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

    // 服务器建造者类
    class ServerBuilder
    {
    public:
        ServerBuilder();
        
        // 注册配置服务
        ServerBuilder& with_config_service(const std::string& config_file = "../config/config.json");
        
        // 注册存储服务
        ServerBuilder& with_storage_services();
        
        // 注册管理器服务
        ServerBuilder& with_manager_services();
        
        // 注册认证服务
        ServerBuilder& with_auth_services();
        
        // 注册处理器工厂
        ServerBuilder& with_handler_factory(interfaces::ICompress::ptr compress);
        
        // 初始化数据库连接池
        ServerBuilder& with_database_pools();
        
        // 构建服务器
        BackupServer::ptr build();
        
        // 一键配置所有服务
        ServerBuilder& with_all_services(const std::string& config_file = "../config/config.json");

    private:
        void ensure_config_service(const std::string& config_file);
        
        interfaces::ICompress::ptr compress_;
        bool config_registered_;
        bool storage_registered_;
        bool managers_registered_;
        bool auth_registered_;
        bool handler_factory_registered_;
        bool database_pools_initialized_;
    };
}
