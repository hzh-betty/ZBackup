#pragma once
#include <string>
#include <memory>
#include "../../../ZHttpServer/include/http/http_request.h"
#include "../../../ZHttpServer/include/http/http_response.h"

namespace zbackup::interfaces
{
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
}
