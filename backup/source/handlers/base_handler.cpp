#include <utility>
#include <ctime>
#include "log/backup_logger.h"
#include "handlers/base_handler.h"
#include "core/service_container.h"

namespace zbackup
{
    std::string BaseHandler::get_etag(const info::BackupInfo &info)
    {
        std::string etag = info.real_path_ + std::to_string(info.fsize_) + std::to_string(info.mtime_);
        return std::to_string(std::hash<std::string>{}(etag));
    }

    std::string BaseHandler::time_to_str(time_t t)
    {
        char buffer[32];
        struct tm tm_info;
        localtime_r(&t, &tm_info);
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &tm_info);
        return std::string(buffer);
    }

    DataHandler::DataHandler(interfaces::IDataManager::ptr data_manager)
        : data_manager_(std::move(data_manager))
    {
        if (!data_manager_)
        {
            auto &container = core::ServiceContainer::get_instance();
            data_manager_ = container.resolve<interfaces::IDataManager>();
            if (!data_manager_)
            {
                ZBACKUP_LOG_ERROR("DataManager not available in service container");
                throw std::runtime_error("DataManager dependency not satisfied");
            }
        }
    }

    CompressHandler::CompressHandler(interfaces::IDataManager::ptr data_manager,
                                     interfaces::ICompress::ptr comp)
        : DataHandler(std::move(data_manager)), comp_(std::move(comp))
    {
        if (!comp_)
        {
            ZBACKUP_LOG_ERROR("Compress component not provided");
            throw std::invalid_argument("Compress component cannot be null");
        }
    }
}
