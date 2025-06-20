#include "server/server.h"
#include "core/service_container.h"
#include "middleware/auth_middleware.h"
#include "core/threadpool.h"
#include "log/backup_logger.h"


namespace zbackup
{
    BackupServer::BackupServer(const core::DependencyConfig& config)
        : dependency_injector_(std::make_unique<core::DependencyInjector>(config)), running_(false)
    {
        ZBACKUP_LOG_DEBUG("BackupServer created with dependency injector");
    }

    BackupServer::ptr BackupServer::create_with_default_config()
    {
        return std::make_shared<BackupServer>(core::DependencyConfig{}.with_database_storage());
    }

    BackupServer::ptr BackupServer::create_with_file_storage()
    {
        return std::make_shared<BackupServer>(core::DependencyConfig{}.with_file_storage());
    }

    BackupServer::ptr BackupServer::create_with_database_storage()
    {
        return std::make_shared<BackupServer>(core::DependencyConfig{}.with_database_storage());
    }

    BackupServer::ptr BackupServer::create_with_config_file(const std::string& config_file)
    {
        return std::make_shared<BackupServer>(
            core::DependencyConfig{}.with_config_file(config_file).with_database_storage()
        );
    }

    bool BackupServer::initialize()
    {
        try
        {
            inject_dependencies();
            resolve_dependencies();
            initialize_server();
            setup_routes();
            return true;
        }
        catch (const std::exception &e)
        {
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
        if (!initialize())
        {
            ZBACKUP_LOG_FATAL("Server initialization failed");
            return;
        }

        running_ = true;
        ZBACKUP_LOG_INFO("BackupServer starting on port {}", config_manager_->get_port());

        core::ThreadPool::get_instance()->start();
        looper_->start();
        server_->start();
    }

    void BackupServer::inject_dependencies()
    {
        ZBACKUP_LOG_DEBUG("Injecting dependencies...");
        dependency_injector_->inject_dependencies();
        ZBACKUP_LOG_DEBUG("Dependencies injected successfully");
    }

    void BackupServer::resolve_dependencies()
    {
        ZBACKUP_LOG_DEBUG("Resolving dependencies from injector");
        
        config_manager_ = dependency_injector_->get_config_manager();
        route_registry_ = dependency_injector_->get_route_registry();
        compressor_ = dependency_injector_->get_compressor();
        zhttp::zsession::SessionManager::get_instance().set_session_storage(std::make_shared<zhttp::zsession::DbSessionStorage>());


        if (!config_manager_ || !route_registry_ || !compressor_)
        {
            ZBACKUP_LOG_FATAL("Required services not available from dependency injector");
            throw std::runtime_error("Service dependencies not satisfied");
        }

        // 创建BackupLooper
        looper_ = std::make_shared<BackupLooper>(compressor_);

        ZBACKUP_LOG_INFO("BackupServer dependencies resolved, will run on {}:{}",
                         config_manager_->get_ip(), config_manager_->get_port());
    }

    void BackupServer::initialize_server()
    {
        auto builder = std::make_unique<zhttp::HttpServerBuilder>();
        builder->build_cert_file_path(config_manager_->get_string("cert_file_path"));
        builder->build_key_file_path(config_manager_->get_string("key_file_path"));
        builder->build_use_ssl(config_manager_->get_bool("use_ssl"));
        builder->build_port(config_manager_->get_port());
        builder->build_name("BackupServer");
        builder->build_middleware(std::make_shared<AuthMiddleware>());
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
