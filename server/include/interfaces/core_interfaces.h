#pragma once
#include <memory>
#include <string>
#include <functional>
#include "../../../ZHttpServer/include/http/http_request.h"
#include "../../../ZHttpServer/include/http/http_response.h"
#include "../../../ZHttpServer/include/router/router_handler.h"

namespace zbackup::interfaces
{
    // 配置管理接口
    class IConfigManager
    {
    public:
        using ptr = std::shared_ptr<IConfigManager>;
        virtual ~IConfigManager() = default;

        virtual bool load_config() = 0;
        virtual std::string get_string(const std::string& key, const std::string& default_value = "") = 0;
        virtual int get_int(const std::string& key, int default_value = 0) = 0;
        virtual bool get_bool(const std::string& key, bool default_value = false) = 0;
        virtual uint16_t get_port() const = 0;
        virtual std::string get_ip() const = 0;
        virtual std::string get_download_prefix() const = 0;
    };

    // 认证服务接口
    class IAuthenticationService
    {
    public:
        using ptr = std::shared_ptr<IAuthenticationService>;
        virtual ~IAuthenticationService() = default;

        virtual bool authenticate(const zhttp::HttpRequest& req) = 0;
        virtual bool login(const std::string& username, const std::string& password, 
                          const zhttp::HttpRequest& req, zhttp::HttpResponse* rsp) = 0;
        virtual bool logout(const zhttp::HttpRequest& req, zhttp::HttpResponse* rsp) = 0;
    };

    // 会话管理接口
    class ISessionManager
    {
    public:
        using ptr = std::shared_ptr<ISessionManager>;
        virtual ~ISessionManager() = default;

        virtual void create_session(const std::string& username, 
                                   const zhttp::HttpRequest& req, zhttp::HttpResponse* rsp) = 0;
        virtual void destroy_session(const zhttp::HttpRequest& req, zhttp::HttpResponse* rsp) = 0;
        virtual std::string get_username(const zhttp::HttpRequest& req) = 0;
        virtual bool is_valid_session(const zhttp::HttpRequest& req) = 0;
    };

    // 请求处理器工厂接口
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

    // 服务器生命周期管理接口
    class IServerLifecycle
    {
    public:
        using ptr = std::shared_ptr<IServerLifecycle>;
        virtual ~IServerLifecycle() = default;

        virtual bool initialize() = 0;
        virtual void start() = 0;
        virtual void stop() = 0;
        virtual bool is_running() const = 0;
    };
}
