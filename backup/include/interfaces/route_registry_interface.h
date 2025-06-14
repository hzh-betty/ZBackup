#pragma once
#include "http/http_server.h"
#include <memory>

namespace zbackup::interfaces
{
    // 路由注册策略接口
    class IRouteRegistry
    {
    public:
        using ptr = std::shared_ptr<IRouteRegistry>;
        virtual ~IRouteRegistry() = default;

        virtual void register_routes(zhttp::HttpServer* server) = 0;
    };
}
