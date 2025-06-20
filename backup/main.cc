#include "server/server.h"
#include <muduo/base/Logging.h>
#include "log/backup_logger.h"
#include "log/http_logger.h"
#include "core/threadpool.h"

int main()
{
    // 初始化日志系统
    muduo::Logger::setLogLevel(muduo::Logger::ERROR);
    zhttp::Log::Init(zlog::LogLevel::value::INFO);
    zbackup::Log::Init(zlog::LogLevel::value::INFO);

    try {
        ZBACKUP_LOG_INFO("ZBackup server starting...");
        
        // 创建服务器 - 使用默认配置（数据库存储）
        auto server = zbackup::BackupServer::create_with_default_config();

        ZBACKUP_LOG_INFO("Server created successfully, starting...");
        server->run();
    }
    catch (const std::exception &e) {
        ZBACKUP_LOG_FATAL("Server startup failed: {}", e.what());
        return -1;
    }

    return 0;
}
