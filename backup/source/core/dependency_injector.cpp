#include "core/dependency_injector.h"
#include "core/service_container.h"
#include "core/config_service.h"
#include "core/authentication_service.h"
#include "core/session_service.h"
#include "core/handler_factory.h"
#include "core/route_registry.h"
#include "data/data_manager.h"
#include "user/user_manager.h"
#include "storage/database/database_backup_storage.h"
#include "storage/database/database_user_storage.h"
#include "storage/file/file_backup_storage.h"
#include "storage/file/file_user_storage.h"
#include "compress/snappy_compress.h"
#include "util/util.h"
#include "log/backup_logger.h"

namespace zbackup::core
{
    DependencyInjector::DependencyInjector(const DependencyConfig& config)
        : config_(config)
    {
        ZBACKUP_LOG_INFO("DependencyInjector created with config: file={}, database={}", 
                         config_.config_file, config_.use_database_storage);
    }

    void DependencyInjector::inject_dependencies()
    {
        ZBACKUP_LOG_INFO("Starting dependency injection process...");
        
        try {
            step1_create_and_register_config_manager();
            step2_initialize_database_pools();
            step3_create_and_register_storage_layer();
            step4_create_and_register_manager_layer();
            step5_create_and_register_compressor();
            step6_create_and_register_auth_layer();
            step7_create_and_register_factory_and_registry();
            
            ZBACKUP_LOG_INFO("Dependency injection completed successfully");
        }
        catch (const std::exception& e) {
            ZBACKUP_LOG_FATAL("Dependency injection failed: {}", e.what());
            throw;
        }
    }

    void DependencyInjector::step1_create_and_register_config_manager()
    {
        ZBACKUP_LOG_DEBUG("Step 1: Creating and registering ConfigManager");
        
        create_config_manager();
        
        auto& container = ServiceContainer::get_instance();
        container.register_instance<interfaces::IConfigManager>(config_manager_);
        
        ZBACKUP_LOG_DEBUG("ConfigManager registered to container");
    }

    void DependencyInjector::step2_initialize_database_pools()
    {
        if (!config_.initialize_database_pools) {
            ZBACKUP_LOG_DEBUG("Step 2: Skipping database pool initialization");
            return;
        }
        
        ZBACKUP_LOG_DEBUG("Step 2: Initializing database pools");
        
        try {
            util::DataBaseInit::InitMysqlPool();
            util::DataBaseInit::InitRedisPool();
            ZBACKUP_LOG_DEBUG("Database pools initialized successfully");
        }
        catch (const std::exception& e) {
            ZBACKUP_LOG_ERROR("Failed to initialize database pools: {}", e.what());
            throw;
        }
    }

    void DependencyInjector::step3_create_and_register_storage_layer()
    {
        ZBACKUP_LOG_DEBUG("Step 3: Creating and registering storage layer");
        
        create_storage_components();
        
        auto& container = ServiceContainer::get_instance();
        container.register_instance<interfaces::IBackupStorage>(backup_storage_);
        container.register_instance<interfaces::IUserStorage>(user_storage_);
        
        ZBACKUP_LOG_DEBUG("Storage layer registered to container");
    }

    void DependencyInjector::step4_create_and_register_manager_layer()
    {
        ZBACKUP_LOG_DEBUG("Step 4: Creating and registering manager layer");
        
        create_manager_components();
        
        auto& container = ServiceContainer::get_instance();
        container.register_instance<interfaces::IDataManager>(data_manager_);
        container.register_instance<interfaces::IUserManager>(user_manager_);
        
        ZBACKUP_LOG_DEBUG("Manager layer registered to container");
    }

    void DependencyInjector::step5_create_and_register_compressor()
    {
        ZBACKUP_LOG_DEBUG("Step 5: Creating and registering compressor");
        
        compressor_ = std::make_shared<SnappyCompress>();
        
        auto& container = ServiceContainer::get_instance();
        container.register_instance<interfaces::ICompress>(compressor_);
        
        ZBACKUP_LOG_DEBUG("Compressor registered to container");
    }

    void DependencyInjector::step6_create_and_register_auth_layer()
    {
        ZBACKUP_LOG_DEBUG("Step 6: Creating and registering auth layer");
        
        create_auth_components();
        
        auto& container = ServiceContainer::get_instance();
        container.register_instance<interfaces::ISessionManager>(session_manager_);
        container.register_instance<interfaces::IAuthenticationService>(auth_service_);
        
        ZBACKUP_LOG_DEBUG("Auth layer registered to container");
    }

    void DependencyInjector::step7_create_and_register_factory_and_registry()
    {
        ZBACKUP_LOG_DEBUG("Step 7: Creating and registering factory and registry");
        
        create_factory_and_registry();
        
        auto& container = ServiceContainer::get_instance();
        container.register_instance<interfaces::IHandlerFactory>(handler_factory_);
        container.register_instance<interfaces::IRouteRegistry>(route_registry_);
        
        ZBACKUP_LOG_DEBUG("Factory and registry registered to container");
    }

    void DependencyInjector::create_config_manager()
    {
        config_manager_ = std::make_shared<ConfigService>(config_.config_file);
    }

    void DependencyInjector::create_storage_components()
    {
        if (config_.use_database_storage) {
            backup_storage_ = std::make_shared<storage::DatabaseBackupStorage>();
            user_storage_ = std::make_shared<storage::DatabaseUserStorage>();
            ZBACKUP_LOG_INFO("Using database storage for backups and users");
        } else {
            backup_storage_ = std::make_shared<storage::FileBackupStorage>();
            user_storage_ = std::make_shared<storage::FileUserStorage>();
        }
    }

    void DependencyInjector::create_manager_components()
    {
        data_manager_ = std::make_shared<DataManager>(backup_storage_);
        user_manager_ = std::make_shared<UserManager>(user_storage_);
    }

    void DependencyInjector::create_auth_components()
    {
        session_manager_ = std::make_shared<SessionService>();
        auth_service_ = std::make_shared<AuthenticationService>(user_manager_, session_manager_);
    }

    void DependencyInjector::create_factory_and_registry()
    {
        handler_factory_ = std::make_shared<HandlerFactory>();
        route_registry_ = std::make_shared<DefaultRouteRegistry>(handler_factory_, config_manager_);
    }

    interfaces::IConfigManager::ptr DependencyInjector::get_config_manager() const
    {
        return config_manager_;
    }

    interfaces::IRouteRegistry::ptr DependencyInjector::get_route_registry() const
    {
        return route_registry_;
    }

    interfaces::ICompress::ptr DependencyInjector::get_compressor() const
    {
        return compressor_;
    }
}
