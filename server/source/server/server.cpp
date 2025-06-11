#include "../../include/server/server.h"
#include "../../include/core/service_container.h"
#include "../../include/core/route_registry.h"
#include "../../include/middleware/auth_middleware.h"
#include "../../../ZHttpServer/include/session/session_manager.h"
#include "../../../ZHttpServer/include/db_pool/redis_pool.h"

namespace zbackup
{
    BackupServer::BackupServer(const Compress::ptr &comp, const Storage::ptr &storage)
        : looper_(std::make_shared<BackupLooper>(comp)), running_(false)
    {
        auto& container = core::ServiceContainer::get_instance();
        
        // 获取依赖服务
        config_manager_ = container.resolve<interfaces::IConfigManager>();
        auto handler_factory = container.resolve<interfaces::IHandlerFactory>();
        
        if (!config_manager_ || !handler_factory) {
            ZBACKUP_LOG_FATAL("Required services not available from container");
            throw std::runtime_error("Service dependencies not satisfied");
        }

        // 创建路由注册器
        route_registry_ = std::make_shared<core::DefaultRouteRegistry>(handler_factory, config_manager_);

        // 初始化数据管理器并设置存储后端
        auto* data_manager = DataManager::get_instance();
        data_manager->initialize_storage(storage);

        // 初始化会话管理器
        zhttp::zsession::SessionManager::get_instance()
                .set_session_storage(zhttp::zsession::StorageFactory<zhttp::zsession::DbSessionStorage>::create());

        ZBACKUP_LOG_INFO("BackupServer initialized on {}:{}", 
                         config_manager_->get_ip(), config_manager_->get_port());
    }

    bool BackupServer::initialize()
    {
        try {
            initialize_server();
            setup_routes();
            return true;
        }
        catch (const std::exception& e) {
            ZBACKUP_LOG_ERROR("Failed to initialize server: {}", e.what());
            return false;
        }
    }

    void BackupServer::start()
    {
        run();
    }

    void BackupServer::stop()
    {
        running_ = false;
        // TODO: 实现优雅停机逻辑
        ZBACKUP_LOG_INFO("BackupServer stop requested");
    }

    bool BackupServer::is_running() const
    {
        return running_;
    }

    void BackupServer::run()
    {
        if (!initialize()) {
            ZBACKUP_LOG_FATAL("Server initialization failed");
            return;
        }

        running_ = true;
        ZBACKUP_LOG_INFO("BackupServer starting on port {}", config_manager_->get_port());
        
        ThreadPool::get_instance()->start();
        looper_->start();
        server_->start();
    }

    void BackupServer::initialize_server()
    {
        auto builder = std::make_unique<zhttp::HttpServerBuilder>();
        builder->build_cert_file_path(config_manager_->get_string("cert_file_path"));
        builder->build_key_file_path(config_manager_->get_string("key_file_path"));
        builder->build_use_ssl(config_manager_->get_bool("use_ssl"));
        builder->build_port(config_manager_->get_port());
        builder->build_name("BackupServer");
        builder->build_middleware(
           std::make_shared<AuthMiddleware>());
        builder->build_thread_num(4);
        server_ = builder->build();
        ZBACKUP_LOG_INFO("BackupServer initialized with SSL: {}, Port: {}, Name: {}",
                         config_manager_->get_bool("use_ssl"),
                         config_manager_->get_port(),
                         "BackupServer");
    }

    void BackupServer::setup_routes()
    {
        route_registry_->register_routes(server_.get());
        ZBACKUP_LOG_INFO("Routes registered successfully");
    }
}
