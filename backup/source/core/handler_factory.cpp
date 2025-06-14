#include "core/handler_factory.h"
#include "core/service_container.h"
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
    HandlerFactory::HandlerFactory(interfaces::ICompress::ptr compress)
        : compress_(std::move(compress))
    {
        auto& container = ServiceContainer::get_instance();
        data_manager_ = container.resolve<interfaces::IDataManager>();
        user_manager_ = container.resolve<interfaces::IUserManager>();
        
        if (!data_manager_) {
            ZBACKUP_LOG_FATAL("DataManager not available in service container");
            throw std::runtime_error("DataManager dependency not satisfied");
        }
        
        if (!user_manager_) {
            ZBACKUP_LOG_FATAL("UserManager not available in service container");
            throw std::runtime_error("UserManager dependency not satisfied");
        }
        
        ZBACKUP_LOG_INFO("HandlerFactory initialized");
    }

    HandlerFactory::HandlerPtr HandlerFactory::create_upload_handler()
    {
        return std::make_shared<UploadHandler>(data_manager_);
    }

    HandlerFactory::HandlerPtr HandlerFactory::create_download_handler()
    {
        return std::make_shared<DownloadHandler>(data_manager_, compress_);
    }

    HandlerFactory::HandlerPtr HandlerFactory::create_list_handler()
    {
        return std::make_shared<ListShowHandler>(data_manager_);
    }

    HandlerFactory::HandlerPtr HandlerFactory::create_delete_handler()
    {
        return std::make_shared<DeleteHandler>(data_manager_);
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
