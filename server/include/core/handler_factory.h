#pragma once
#include "../interfaces/handler_factory_interface.h"
#include "../interfaces/auth_manager_interface.h"
#include "../interfaces/compress_interface.h"
#include "../user/user_manager.h"

namespace zbackup::core
{
    class HandlerFactory : public interfaces::IHandlerFactory
    {
    public:
        using HandlerPtr = zhttp::zrouter::RouterHandler::ptr;

    public:
        explicit HandlerFactory(interfaces::ICompress::ptr compress);
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
        interfaces::ICompress::ptr compress_;
        interfaces::IAuthenticationService::ptr auth_service_;
        std::shared_ptr<UserManager> user_manager_;
    };
}
