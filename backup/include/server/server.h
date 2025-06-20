#pragma once
#include "looper.h"
#include "interfaces/server_lifecycle_interface.h"
#include "interfaces/config_manager_interface.h"
#include "interfaces/route_registry_interface.h"
#include "interfaces/compress_interface.h"
#include "core/dependency_injector.h"
#include "http/http_server.h"
#include <memory>
#include <atomic>

namespace zbackup
{
    class BackupServer : public interfaces::IServerLifecycle
    {
    public:
        using ptr = std::shared_ptr<BackupServer>;
        
        // 构造函数 - 使用依赖注入配置
        explicit BackupServer(const core::DependencyConfig& config = core::DependencyConfig{});
        
        // 静态工厂方法
        static ptr create_with_default_config();
        static ptr create_with_file_storage();
        static ptr create_with_database_storage();
        static ptr create_with_config_file(const std::string& config_file);

        // IServerLifecycle 接口实现
        bool initialize() override;
        void start() override;
        void stop() override;
        bool is_running() const override;

        void run();

    private:
        void inject_dependencies();
        void resolve_dependencies();
        void initialize_server();
        void setup_routes();

        // 依赖注入管理器
        std::unique_ptr<core::DependencyInjector> dependency_injector_;
        
        // 服务器组件
        std::unique_ptr<zhttp::HttpServer> server_;
        BackupLooper::ptr looper_;
        std::atomic<bool> running_;

        // 主要依赖服务
        interfaces::IConfigManager::ptr config_manager_;
        interfaces::IRouteRegistry::ptr route_registry_;
        interfaces::ICompress::ptr compressor_;
    };
} // namespace zbackup