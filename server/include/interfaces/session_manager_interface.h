#pragma once
#include <string>
#include <memory>
#include "../../../ZHttpServer/include/http/http_request.h"
#include "../../../ZHttpServer/include/http/http_response.h"

namespace zbackup::interfaces
{
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
}
