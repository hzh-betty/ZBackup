#pragma once
#include "../interfaces/core_interfaces.h"
#include "../../../ZHttpServer/include/http/http_server.h"
#include <memory>

namespace zbackup::core
{
    // 路由注册策略接口
    class IRouteRegistry
    {
    public:
        using ptr = std::shared_ptr<IRouteRegistry>;
        virtual ~IRouteRegistry() = default;

        virtual void register_routes(zhttp::HttpServer* server) = 0;
    };

    // 默认路由注册实现
    class DefaultRouteRegistry : public IRouteRegistry
    {
    public:
        explicit DefaultRouteRegistry(interfaces::IHandlerFactory::ptr handler_factory,
                                     interfaces::IConfigManager::ptr config_manager);

        void register_routes(zhttp::HttpServer* server) override;

    private:
        void register_public_routes(zhttp::HttpServer* server);
        void register_auth_routes(zhttp::HttpServer* server);
        void register_business_routes(zhttp::HttpServer* server);
        
        std::function<void(const zhttp::HttpRequest&, zhttp::HttpResponse*)> create_status_handler();
        std::function<void(const zhttp::HttpRequest&, zhttp::HttpResponse*)> create_redirect_handler();

        interfaces::IHandlerFactory::ptr handler_factory_;
        interfaces::IConfigManager::ptr config_manager_;
    };
}
