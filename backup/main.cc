#include "server/server.h"
#include "core/service_container.h"
#include "core/config_service.h"
#include "core/authentication_service.h"
#include "core/session_service.h"
#include "core/handler_factory.h"
#include "data/data_manager.h"
#include "user/user_manager.h"
#include "compress/snappy_compress.h"
#include "storage/database/database_backup_storage.h"
#include "storage/database/database_user_storage.h"
#include <muduo/base/Logging.h>
#include "log/backup_logger.h"
#include "log/http_logger.h"
#include "util/util.h"

void setup_dependencies()
{
    auto& container = zbackup::core::ServiceContainer::get_instance();
    
    // 1. 首先注册配置服务（其他服务都依赖它）
    auto config_service = std::make_shared<zbackup::core::ConfigService>();
    container.register_instance<zbackup::interfaces::IConfigManager>(config_service);
    ZBACKUP_LOG_INFO("ConfigService registered successfully");
    
    //  2. 初始化数据库连接池（需要配置服务）
    zbackup::util::DataBaseInit::InitMysqlPool();
    zbackup::util::DataBaseInit::InitMysqlPool();

    // 3. 注册存储层（需要数据库配置）
    container.register_singleton<zbackup::interfaces::IBackupStorage, zbackup::storage::DatabaseBackupStorage>();
    container.register_singleton<zbackup::interfaces::IUserStorage, zbackup::storage::DatabaseUserStorage>();
    
    // 4. 注册管理器（依赖存储层）
    auto data_manager = std::make_shared<zbackup::DataManager>(
        container.resolve<zbackup::interfaces::IBackupStorage>());
    container.register_instance<zbackup::interfaces::IDataManager>(data_manager);
    
    auto user_manager = std::make_shared<zbackup::UserManager>(
        container.resolve<zbackup::interfaces::IUserStorage>());
    container.register_instance<zbackup::interfaces::IUserManager>(user_manager);
    
    // 4. 注册会话管理器
    container.register_singleton<zbackup::interfaces::ISessionManager, zbackup::core::SessionService>();
    
    // 5. 注册认证服务（依赖用户管理器和会话管理器）
    container.register_singleton<zbackup::interfaces::IAuthenticationService, zbackup::core::AuthenticationService>();
    
    ZBACKUP_LOG_INFO("Service dependencies configured successfully");
}

int main()
{
    // 初始化日志系统
    muduo::Logger::setLogLevel(muduo::Logger::ERROR);
    zhttp::Log::Init(zlog::LogLevel::value::INFO);
    zbackup::Log::Init(zlog::LogLevel::value::INFO);

    try {
        // 1. 设置依赖注入（在使用配置之前）
        setup_dependencies();
        
        ZBACKUP_LOG_INFO("ZBackup server starting...");

        // 2. 创建核心组件
        auto& container = zbackup::core::ServiceContainer::get_instance();
        zbackup::interfaces::ICompress::ptr compress(new zbackup::SnappyCompress());
        
        // 3. 注册处理器工厂
        auto handler_factory = std::make_shared<zbackup::core::HandlerFactory>(compress);
        container.register_instance<zbackup::interfaces::IHandlerFactory>(handler_factory);
        
        // 4. 创建服务器
        auto storage = container.resolve<zbackup::interfaces::IBackupStorage>();
        zbackup::BackupServer::ptr server(new zbackup::BackupServer(compress, storage));

        ZBACKUP_LOG_INFO("Thread pool started successfully");
        server->run();
    }
    catch (const std::exception &e) {
        ZBACKUP_LOG_FATAL("Server startup failed: {}", e.what());
        return -1;
    }

    return 0;
}

