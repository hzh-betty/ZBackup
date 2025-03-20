#pragma once
#include "../ZLog/zlog/zlog.h"
namespace zbackup
{
    zlog::Logger::ptr logger;
    class Log
    {
    public:
        static void Init(zlog::LogLevel::value limitLevel = zlog::LogLevel::value::DEBUG)
        {
            std::unique_ptr<zlog::GlobalLoggerBuilder> builder(new zlog::GlobalLoggerBuilder());
            builder->buildLoggerName("async_logger");
            builder->buildLoggerLevel(limitLevel);
            builder->buildLoggerFormmater("[%d][%f:%l]%T[%p] %m%n");
            // builder->buildLoggerType(zlog::LoggerType::LOGGER_ASYNC);
            // builder->buildWaitTime(std::chrono::milliseconds(100));
            builder->buildLoggerSink<zlog::StdOutSink>();
            logger = builder->build();
        }
    };
};