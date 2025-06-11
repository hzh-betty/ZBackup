#pragma once
#include "../../../ZHttpServer/include/router/router_handler.h"
#include "../util/util.h"
#include "../log/logger.h"
#include "../../include/data/data_manage.h"

namespace zbackup
{
    struct BackupInfo;
    class DataManager;
    class Compress;

    // 基础处理器接口
    class IHandler : public zhttp::zrouter::RouterHandler
    {
    public:
        ~IHandler() override = default;
        void handle_request(const zhttp::HttpRequest &req, zhttp::HttpResponse *rsp) override {}
    };

    // 通用处理器基类
    class BaseHandler : public IHandler
    {
    protected:
        BaseHandler() = default;
        // 获取 ETag - 静态工具方法
        static std::string get_etag(const BackupInfo &info);

        // 时间转字符串 - 静态工具方法
        static std::string time_to_str(time_t t);
    };

    // 需要数据管理的处理器基类
    class DataHandler : public BaseHandler
    {
    protected:
        DataHandler();
        DataManager* data_manager_ = DataManager::get_instance();
    };

    // 需要压缩功能的处理器基类
    class CompressHandler : public DataHandler
    {
    protected:        
        explicit CompressHandler(std::shared_ptr<Compress> comp);
        std::shared_ptr<Compress> comp_;
    };
}
