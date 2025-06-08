#pragma once
#include "../../ZHttpServer/include/router/router_handler.h"
#include "../compress/compress.h"
#include "../data/data_manage.h"
#include "../util/util.h"
#include "../log/logger.h"

namespace zbackup
{
    class BaseHandler : public zhttp::zrouter::RouterHandler
    {
    protected:
        BaseHandler(Compress::ptr comp);
        virtual void handle_request(const zhttp::HttpRequest &req, zhttp::HttpResponse *rsp){}

        // 获取 ETag
        static std::string get_etag(const BackupInfo &info);

        // 时间转字符串
        static std::string time_to_str(time_t t);

        Compress::ptr comp_;
        DataManager* data_manager_;
    };
}
