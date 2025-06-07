#pragma once
#include "../ZHttpServer/zlog/zlog.h"
namespace zbackup
{
    inline zlog::Logger::ptr logger;
    class Log
    {
    public:
        static void Init(zlog::LogLevel::value limitLevel = zlog::LogLevel::value::DEBUG)
        {
            std::unique_ptr<zlog::GlobalLoggerBuilder> builder(new zlog::GlobalLoggerBuilder());
            builder->buildLoggerName("backup_logger");
            builder->buildLoggerLevel(limitLevel);
            builder->buildLoggerFormmater("[%d][%f:%l[%p][%t]  %m%n");
            builder->buildLoggerSink<zlog::StdOutSink>();
            logger = builder->build();
        }
    };
};