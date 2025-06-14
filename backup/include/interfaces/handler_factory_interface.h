#pragma once
#include "router/router_handler.h"
#include "compress_interface.h"
#include <memory>

namespace zbackup::interfaces
{
    // 处理器工厂接口
    class IHandlerFactory
    {
    public:
        using ptr = std::shared_ptr<IHandlerFactory>;
        using HandlerPtr = std::shared_ptr<zhttp::zrouter::RouterHandler>;
        virtual ~IHandlerFactory() = default;

        virtual HandlerPtr create_upload_handler() = 0;
        virtual HandlerPtr create_download_handler() = 0;
        virtual HandlerPtr create_list_handler() = 0;
        virtual HandlerPtr create_delete_handler() = 0;
        virtual HandlerPtr create_static_handler() = 0;
        virtual HandlerPtr create_login_handler() = 0;
        virtual HandlerPtr create_register_handler() = 0;
        virtual HandlerPtr create_logout_handler() = 0;
    };
}
