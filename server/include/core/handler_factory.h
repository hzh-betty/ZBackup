#pragma once
#include "../interfaces/core_interfaces.h"
#include "../compress/compress.h"
#include "../user/user_manager.h"

namespace zbackup::core
{
    class HandlerFactory : public interfaces::IHandlerFactory
    {
    public:
        using HandlerPtr = std::shared_ptr<zhttp::zrouter::RouterHandler>;

    public:
        explicit HandlerFactory(Compress::ptr compress);
        ~HandlerFactory() override = default;

        HandlerPtr create_upload_handler() override;
        HandlerPtr create_download_handler() override;
        HandlerPtr create_list_handler() override;
        HandlerPtr create_delete_handler() override;
        HandlerPtr create_static_handler() override;
        HandlerPtr create_login_handler() override;
        HandlerPtr create_register_handler() override;
        HandlerPtr create_logout_handler() override;

    private:
        Compress::ptr compress_;
        interfaces::IAuthenticationService::ptr auth_service_;
        std::shared_ptr<UserManager> user_manager_;
    };
}
