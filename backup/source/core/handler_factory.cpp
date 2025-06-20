#include "core/handler_factory.h"
#include "handlers/upload_handler.h"
#include "handlers/download_handler.h"
#include "handlers/listshow_handler.h"
#include "handlers/delete_handler.h"
#include "handlers/static_handler.h"
#include "handlers/login_handler.h"
#include "handlers/register_handler.h"
#include "handlers/logout_handler.h"
#include "log/backup_logger.h"

namespace zbackup::core
{
    HandlerFactory::HandlerPtr HandlerFactory::create_upload_handler()
    {
        return std::make_shared<UploadHandler>();
    }

    HandlerFactory::HandlerPtr HandlerFactory::create_download_handler()
    {
        return std::make_shared<DownloadHandler>();
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
        return std::make_shared<LoginHandler>();
    }

    HandlerFactory::HandlerPtr HandlerFactory::create_register_handler()
    {
        return std::make_shared<RegisterHandler>();
    }

    HandlerFactory::HandlerPtr HandlerFactory::create_logout_handler()
    {
        return std::make_shared<LogoutHandler>();
    }
}
   