#include "include/server/server.h"
#include "include/core/service_container.h"
#include "include/core/config_service.h"
#include "include/core/authentication_service.h"
#include "include/core/session_service.h"
#include "include/core/handler_factory.h"
#include "include/user/user_manager.h"
#include "include/compress/snappy_compress.h"
#include <muduo/base/Logging.h>
#include "include/log/logger.h"
#include "../ZHttpServer/include/log/logger.h"

void setup_dependencies()
{
    auto& container = zbackup::core::ServiceContainer::get_instance();
    
    // 注册配置服务
    container.register_singleton<zbackup::interfaces::IConfigManager, zbackup::core::ConfigService>();
    
    // 注册用户管理器
    container.register_singleton<zbackup::UserManager, zbackup::UserManager>();
    
    // 注册会话管理器
    container.register_singleton<zbackup::interfaces::ISessionManager, zbackup::core::SessionService>();
    
    // 注册认证服务
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
        // 设置依赖注入
        setup_dependencies();
        
        // 初始化数据库连接池
        zbackup::InitMysqlPool();
        zbackup::InitRedisPool();
        ZBACKUP_LOG_INFO("ZBackup server starting...");

        // 创建核心组件
        auto& container = zbackup::core::ServiceContainer::get_instance();
        zbackup::interfaces::ICompress::ptr compress(new zbackup::SnappyCompress());
        zbackup::Storage::ptr storage(new zbackup::DatabaseStorage());
        
        // 注册处理器工厂
        auto handler_factory = std::make_shared<zbackup::core::HandlerFactory>(compress);
        container.register_instance<zbackup::interfaces::IHandlerFactory>(handler_factory);
        
        // 创建服务器
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

