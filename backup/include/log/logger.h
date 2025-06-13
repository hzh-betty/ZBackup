#pragma once
#include "../../../ZHttpServer/zlog/zlog.h"

namespace zbackup
{
    inline zlog::Logger::ptr backup_logger;

    class Log
    {
    public:
        static void Init(zlog::LogLevel::value limitLevel = zlog::LogLevel::value::DEBUG)
        {
            std::unique_ptr<zlog::GlobalLoggerBuilder> builder(new zlog::GlobalLoggerBuilder());
            builder->buildLoggerName("backup_logger");
            builder->buildLoggerLevel(limitLevel);
            builder->buildLoggerFormmater("[%c][%d][%f:%l][%p]  %m%n");
            builder->buildLoggerSink<zlog::StdOutSink>();
            backup_logger = builder->build();
        }
    };

    // 便利的日志宏定义 - 使用fmt库格式
#define ZBACKUP_LOG_DEBUG(fmt, ...) zbackup::backup_logger->ZLOG_DEBUG(fmt, ##__VA_ARGS__)
#define ZBACKUP_LOG_INFO(fmt, ...) zbackup::backup_logger->ZLOG_INFO(fmt, ##__VA_ARGS__)
#define ZBACKUP_LOG_WARN(fmt, ...) zbackup::backup_logger->ZLOG_WARN(fmt, ##__VA_ARGS__)
#define ZBACKUP_LOG_ERROR(fmt, ...) zbackup::backup_logger->ZLOG_ERROR(fmt, ##__VA_ARGS__)
#define ZBACKUP_LOG_FATAL(fmt, ...) zbackup::backup_logger->ZLOG_FATAL(fmt, ##__VA_ARGS__)
};
