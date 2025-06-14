#include "core/route_registry.h"
#include "util/util.h"
#include "core/service_container.h"
#include "interfaces/auth_manager_interface.h"
#include "interfaces/session_manager_interface.h"
#include <nlohmann/json.hpp>

namespace zbackup::core
{
    DefaultRouteRegistry::DefaultRouteRegistry(interfaces::IHandlerFactory::ptr handler_factory,
                                               interfaces::IConfigManager::ptr config_manager)
        : handler_factory_(std::move(handler_factory)), config_manager_(std::move(config_manager))
    {
    }

    void DefaultRouteRegistry::register_routes(zhttp::HttpServer *server)
    {
        register_public_routes(server);
        register_auth_routes(server);
        register_business_routes(server);
    }

    void DefaultRouteRegistry::register_public_routes(zhttp::HttpServer *server)
    {
        auto static_handler = handler_factory_->create_static_handler();

        // 注册公共页面路由
        server->Get("/home.html", static_handler);
        server->Get("/home", static_handler);
        server->Get("/api/status", create_status_handler());
        server->Get("/", create_redirect_handler());
    }

    void DefaultRouteRegistry::register_auth_routes(zhttp::HttpServer *server)
    {
        auto static_handler = handler_factory_->create_static_handler();
        auto login_handler = handler_factory_->create_login_handler();
        auto register_handler = handler_factory_->create_register_handler();

        // 注册认证相关路由
        server->Get("/login.html", static_handler);
        server->Get("/register.html", static_handler);
        server->Post("/login", login_handler);
        server->Post("/register", register_handler);
    }

    void DefaultRouteRegistry::register_business_routes(zhttp::HttpServer *server)
    {
        auto static_handler = handler_factory_->create_static_handler();
        auto upload_handler = handler_factory_->create_upload_handler();
        auto list_handler = handler_factory_->create_list_handler();
        auto download_handler = handler_factory_->create_download_handler();
        auto delete_handler = handler_factory_->create_delete_handler();
        auto logout_handler = handler_factory_->create_logout_handler();

        // 注册业务功能路由
        server->Get("/index.html", static_handler);
        server->Get("/index", static_handler);
        server->Post("/upload", upload_handler);
        server->Get("/listshow", list_handler);
        server->Delete("/delete", delete_handler);
        server->Post("/logout", logout_handler);

        // 注册下载路由
        std::string download_url = config_manager_->get_download_prefix() + ":filename";
        server->add_regex_route(zhttp::HttpRequest::Method::GET, download_url, download_handler);
    }

    std::function<void(const zhttp::HttpRequest &, zhttp::HttpResponse *)>
    DefaultRouteRegistry::create_status_handler()
    {
        return [](const zhttp::HttpRequest &req, zhttp::HttpResponse *rsp) {
            nlohmann::json status_json;

            auto &container = ServiceContainer::get_instance();
            auto auth_service = container.resolve<interfaces::IAuthenticationService>();
            auto session_service = container.resolve<interfaces::ISessionManager>();

            try
            {
                if (auth_service && auth_service->authenticate(req))
                {
                    status_json["logged_in"] = true;
                    if (session_service)
                    {
                        status_json["username"] = session_service->get_username(req);
                    }
                }
                else
                {
                    status_json["logged_in"] = false;
                }
            }
            catch (...)
            {
                status_json["logged_in"] = false;
            }

            std::string response_body;
            util::JsonUtil::serialize(status_json, &response_body);

            rsp->set_status_code(zhttp::HttpResponse::StatusCode::OK);
            rsp->set_status_message("OK");
            rsp->set_content_type("application/json");
            rsp->set_body(response_body);
        };
    }

    std::function<void(const zhttp::HttpRequest &, zhttp::HttpResponse *)>
    DefaultRouteRegistry::create_redirect_handler()
    {
        return [](const zhttp::HttpRequest &req, zhttp::HttpResponse *rsp) {
            auto &container = ServiceContainer::get_instance();
            auto auth_service = container.resolve<interfaces::IAuthenticationService>();

            try
            {
                if (auth_service && auth_service->authenticate(req))
                {
                    rsp->set_status_code(zhttp::HttpResponse::StatusCode::Found);
                    rsp->set_status_message("Found");
                    rsp->set_header("Location", "/index.html");
                    return;
                }
            }
            catch (...)
            {
                // 认证检查失败，继续显示首页
            }

            rsp->set_status_code(zhttp::HttpResponse::StatusCode::Found);
            rsp->set_status_message("Found");
            rsp->set_header("Location", "/home.html");
        };
    }
}
