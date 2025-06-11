#include <utility>
#include <ctime>
#include "../../include/log/logger.h"
#include "../../include/handlers/base_handler.h"
#include "../../include/storage/storage.h"
#include "../../include/data/data_manage.h"
#include "../../include/compress/compress.h"

namespace zbackup
{
    std::string BaseHandler::get_etag(const BackupInfo &info)
    {
        FileUtil fu(info.real_path_);
        std::string etag = fu.get_name();
        etag += "-";
        etag += std::to_string(info.fsize_);
        etag += "-";
        etag += std::to_string(info.mtime_);
        ZBACKUP_LOG_DEBUG("Generated ETag: {}", etag);
        return etag;
    }

    std::string BaseHandler::time_to_str(time_t t)
    {
        std::string tmp = std::ctime(&t);
        // 移除末尾的换行符
        if (!tmp.empty() && tmp.back() == '\n') {
            tmp.pop_back();
        }
        return tmp;
    }

    DataHandler::DataHandler()
    {
        data_manager_ = DataManager::get_instance();
    }

    CompressHandler::CompressHandler(std::shared_ptr<Compress> comp) 
        : comp_(std::move(comp))
    {
    }
}
