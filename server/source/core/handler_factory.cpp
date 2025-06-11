#include "../../include/core/handler_factory.h"
#include "../../include/core/service_container.h"
#include "../../include/handlers/upload_handler.h"
#include "../../include/handlers/download_handler.h"
#include "../../include/handlers/listshow_handler.h"
#include "../../include/handlers/delete_handler.h"
#include "../../include/handlers/static_handler.h"
#include "../../include/handlers/login_handler.h"
#include "../../include/handlers/register_handler.h"
#include "../../include/handlers/logout_handler.h"
#include "../../include/log/logger.h"

namespace zbackup::core
{
    HandlerFactory::HandlerFactory(Compress::ptr compress)
        : compress_(std::move(compress))
    {
        auto& container = ServiceContainer::get_instance();
        auth_service_ = container.resolve<interfaces::IAuthenticationService>();
        user_manager_ = container.resolve<UserManager>();
        ZBACKUP_LOG_INFO("HandlerFactory initialized");
    }

    HandlerFactory::HandlerPtr HandlerFactory::create_upload_handler()
    {
        return std::make_shared<UploadHandler>();
    }

    HandlerFactory::HandlerPtr HandlerFactory::create_download_handler()
    {
        return std::make_shared<DownloadHandler>(compress_);
    }

    HandlerFactory::HandlerPtr HandlerFactory::create_list_handler()
    {
        return std::make_shared<ListShowHandler>();
    }

    HandlerFactory::HandlerPtr HandlerFactory::create_delete_handler()
    {
        return std::make_shared<DeleteHandler>();
    }

    HandlerFactory::HandlerPtr HandlerFactory::create_static_handler()
    {
        return std::make_shared<StaticHandler>();
    }

    HandlerFactory::HandlerPtr HandlerFactory::create_login_handler()
    {
        return std::make_shared<LoginHandler>(user_manager_);
    }

    HandlerFactory::HandlerPtr HandlerFactory::create_register_handler()
    {
        return std::make_shared<RegisterHandler>(user_manager_);
    }

    HandlerFactory::HandlerPtr HandlerFactory::create_logout_handler()
    {
        return std::make_shared<LogoutHandler>();
    }
}
