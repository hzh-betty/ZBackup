#include "server/server.h"
#include "core/service_container.h"
#include "core/route_registry.h"
#include "core/config_service.h"
#include "core/authentication_service.h"
#include "core/session_service.h"
#include "core/handler_factory.h"
#include "data/data_manager.h"
#include "user/user_manager.h"
#include "storage/database/database_backup_storage.h"
#include "storage/database/database_user_storage.h"
#include "middleware/auth_middleware.h"
#include "session/session_manager.h"
#include "core/threadpool.h"
#include "util/util.h"
#include "log/backup_logger.h"

namespace zbackup
{
    // ServerBuilder 实现
    ServerBuilder::ServerBuilder()
        : config_registered_(false)
        , storage_registered_(false)
        , managers_registered_(false)
        , auth_registered_(false)
        , handler_factory_registered_(false)
        , database_pools_initialized_(false)
    {
    }

    ServerBuilder& ServerBuilder::with_config_service(const std::string& config_file)
    {
        if (config_registered_) {
            ZBACKUP_LOG_WARN("Config service already registered, skipping");
            return *this;
        }

        auto& container = core::ServiceContainer::get_instance();
        auto config_service = std::make_shared<core::ConfigService>(config_file);
        container.register_instance<interfaces::IConfigManager>(config_service);
        
        config_registered_ = true;
        ZBACKUP_LOG_INFO("Config service registered successfully with file: {}", config_file);
        return *this;
    }

    ServerBuilder& ServerBuilder::with_database_pools()
    {
        if (database_pools_initialized_) {
            ZBACKUP_LOG_WARN("Database pools already initialized, skipping");
            return *this;
        }

        ensure_config_service("../config/config.json");
        
        util::DataBaseInit::InitMysqlPool();
        util::DataBaseInit::InitRedisPool();
        
        database_pools_initialized_ = true;
        ZBACKUP_LOG_INFO("Database connection pools initialized successfully");
        return *this;
    }

    ServerBuilder& ServerBuilder::with_storage_services()
    {
        if (storage_registered_) {
            ZBACKUP_LOG_WARN("Storage services already registered, skipping");
            return *this;
        }

        auto& container = core::ServiceContainer::get_instance();
        
        // 注册存储层服务
        container.register_singleton<interfaces::IBackupStorage, storage::DatabaseBackupStorage>();
        container.register_singleton<interfaces::IUserStorage, storage::DatabaseUserStorage>();
        
        storage_registered_ = true;
        ZBACKUP_LOG_INFO("Storage services registered successfully");
        return *this;
    }

    ServerBuilder& ServerBuilder::with_manager_services()
    {
        if (managers_registered_) {
            ZBACKUP_LOG_WARN("Manager services already registered, skipping");
            return *this;
        }

        auto& container = core::ServiceContainer::get_instance();
        
        // 确保存储服务已注册
        if (!storage_registered_) {
            with_storage_services();
        }
        
        // 注册数据管理器
        auto data_manager = std::make_shared<DataManager>(
            container.resolve<interfaces::IBackupStorage>());
        container.register_instance<interfaces::IDataManager>(data_manager);
        
        // 注册用户管理器
        auto user_manager = std::make_shared<UserManager>(
            container.resolve<interfaces::IUserStorage>());
        container.register_instance<interfaces::IUserManager>(user_manager);
        
        managers_registered_ = true;
        ZBACKUP_LOG_INFO("Manager services registered successfully");
        return *this;
    }

    ServerBuilder& ServerBuilder::with_auth_services()
    {
        if (auth_registered_) {
            ZBACKUP_LOG_WARN("Auth services already registered, skipping");
            return *this;
        }

        auto& container = core::ServiceContainer::get_instance();
        
        // 确保管理器服务已注册
        if (!managers_registered_) {
            with_manager_services();
        }
        
        // 注册会话管理器
        container.register_singleton<interfaces::ISessionManager, core::SessionService>();
        
        // 注册认证服务
        auto user_manager = container.resolve<interfaces::IUserManager>();
        auto session_manager = container.resolve<interfaces::ISessionManager>();
        container.register_instance<interfaces::IAuthenticationService>(
            std::make_shared<core::AuthenticationService>(user_manager, session_manager)
        );
        
        auth_registered_ = true;
        ZBACKUP_LOG_INFO("Authentication services registered successfully");
        return *this;
    }

    ServerBuilder& ServerBuilder::with_handler_factory(interfaces::ICompress::ptr compress)
    {
        if (handler_factory_registered_) {
            ZBACKUP_LOG_WARN("Handler factory already registered, skipping");
            return *this;
        }

        auto& container = core::ServiceContainer::get_instance();
        
        // 保存压缩器引用
        compress_ = compress;
        
        // 注册处理器工厂
        auto handler_factory = std::make_shared<core::HandlerFactory>(compress);
        container.register_instance<interfaces::IHandlerFactory>(handler_factory);
        
        handler_factory_registered_ = true;
        ZBACKUP_LOG_INFO("Handler factory registered successfully");
        return *this;
    }

    ServerBuilder& ServerBuilder::with_all_services(const std::string& config_file)
    {
        return with_config_service(config_file)
               .with_database_pools()
               .with_storage_services()
               .with_manager_services()
               .with_auth_services();
    }

    BackupServer::ptr ServerBuilder::build()
    {
        // 确保所有必要的服务都已注册
        ensure_config_service("../config/config.json");
        
        if (!compress_) {
            ZBACKUP_LOG_ERROR("Compress service not provided, cannot build server");
            throw std::runtime_error("Compress service is required");
        }
        
        if (!handler_factory_registered_) {
            with_handler_factory(compress_);
        }
        
        auto& container = core::ServiceContainer::get_instance();
        auto storage = container.resolve<interfaces::IBackupStorage>();
        
        if (!storage) {
            ZBACKUP_LOG_ERROR("Storage service not available, cannot build server");
            throw std::runtime_error("Storage service is required");
        }
        
        auto server = std::make_shared<BackupServer>(compress_, storage);
        ZBACKUP_LOG_INFO("BackupServer built successfully");
        return server;
    }

    void ServerBuilder::ensure_config_service(const std::string& config_file)
    {
        if (!config_registered_) {
            with_config_service(config_file);
        }
    }

    // BackupServer 实现保持不变
    BackupServer::BackupServer(const interfaces::ICompress::ptr &comp, const interfaces::IBackupStorage::ptr &storage)
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
        
        core::ThreadPool::get_instance()->start();
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
