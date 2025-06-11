#include <utility>

#include "../../include/handlers/base_handler.h"

namespace zbackup
{
    BaseHandler::BaseHandler(Compress::ptr comp) : comp_(std::move(comp))
    {
        data_manager_ = DataManager::get_instance();
    }

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
        if (!tmp.empty() && tmp.back() == '\n')
        {
            tmp.pop_back();
        }
        return tmp;
    }
}
