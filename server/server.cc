#include "include/server/server.h"
#include <muduo/base/Logging.h>


int main()
{
    // 初始化日志系统
    muduo::Logger::setLogLevel(muduo::Logger::ERROR);
    zhttp::Log::Init(zlog::LogLevel::value::INFO);
    zbackup::Log::Init(zlog::LogLevel::value::INFO);
    
    ZBACKUP_LOG_INFO("ZBackup server starting...");
    
    try {
        // 创建核心组件
        zbackup::Compress::ptr compress(new zbackup::SnappyCompress());
        zbackup::Storage::ptr storage(new zbackup::FileStorage());
        zbackup::BackupServer::ptr server(new zbackup::BackupServer(compress, storage));
        
        // 启动服务器
        ZBACKUP_LOG_INFO("Thread pool started successfully");
        
        server->run();
    }
    catch (const std::exception& e) {
        ZBACKUP_LOG_FATAL("Server startup failed: {}", e.what());
        return -1;
    }
    
    return 0;
}