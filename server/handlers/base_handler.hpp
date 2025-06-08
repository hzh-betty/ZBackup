#pragma once
#include "../../ZHttpServer/include/router/router_handler.h"
#include "../compress.hpp"
#include "../data_manage.hpp"
#include "../util.hpp"
#include "../logger.hpp"

namespace zbackup
{
    class BaseHandler : public zhttp::zrouter::RouterHandler
    {
    protected:
        BaseHandler(Compress::ptr comp) : comp_(comp) {}

        // 获取 ETag
        static std::string GetETag(const BackupInfo &info)
        {
            FileUtil fu(info.realPath_);
            std::string etag = fu.getName();
            etag += "-";
            etag += std::to_string(info.fsize_);
            etag += "-";
            etag += std::to_string(info.mtime_);
            logger->debug("create ETag {}", etag);
            return etag;
        }

        // 时间转字符串
        static std::string TimetoStr(time_t t)
        {
            std::string tmp = std::ctime(&t);
            logger->debug("time to str {}", tmp);
            return tmp;
        }

        Compress::ptr comp_;
        DataManager* dataManager_ = DataManager::getInstance();
    };
};
