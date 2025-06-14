#pragma once
#include "router/router_handler.h"
#include "interfaces/compress_interface.h"
#include "interfaces/data_manager_interface.h"

namespace zbackup
{
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
        static std::string get_etag(const info::BackupInfo &info);

        // 时间转字符串 - 静态工具方法
        static std::string time_to_str(time_t t);
    };

    // 需要数据管理的处理器基类
    class DataHandler : public BaseHandler
    {
    protected:
        explicit DataHandler(interfaces::IDataManager::ptr data_manager);
        interfaces::IDataManager::ptr data_manager_;
    };

    // 需要压缩功能的处理器基类
    class CompressHandler : public DataHandler
    {
    protected:
        explicit CompressHandler(interfaces::IDataManager::ptr data_manager, 
                               interfaces::ICompress::ptr comp);
        interfaces::ICompress::ptr comp_;
    };
}

