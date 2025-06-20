#pragma once
#include "interfaces/handler_factory_interface.h"

namespace zbackup::core
{
    class HandlerFactory : public interfaces::IHandlerFactory
    {
    public:
        using HandlerPtr = zhttp::zrouter::RouterHandler::ptr;

    public:
        HandlerFactory() = default;
        ~HandlerFactory() override = default;

        HandlerPtr create_upload_handler() override;
        HandlerPtr create_download_handler() override;
        HandlerPtr create_list_handler() override;
        HandlerPtr create_delete_handler() override;
        HandlerPtr create_static_handler() override;
        HandlerPtr create_login_handler() override;
        HandlerPtr create_register_handler() override;
        HandlerPtr create_logout_handler() override;
    };
}