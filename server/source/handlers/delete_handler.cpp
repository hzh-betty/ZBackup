#include <utility>

#include "../../include/handlers/delete_handler.h"
#include "../../include/util/util.h"
#include <nlohmann/json.hpp>

namespace zbackup
{
    DeleteHandler::DeleteHandler(interfaces::IDataManager::ptr data_manager)
        : DataHandler(std::move(data_manager))
    {
        ZBACKUP_LOG_DEBUG("DeleteHandler initialized");
    }

    void DeleteHandler::handle_request(const zhttp::HttpRequest &req, zhttp::HttpResponse *rsp)
    {
        std::string file_url = req.get_path();
        ZBACKUP_LOG_INFO("Delete request: {}", file_url);

        info::BackupInfo info;
        if (!data_manager_->get_one_by_url(file_url, &info))
        {
            ZBACKUP_LOG_WARN("File not found for deletion: {}", file_url);
            rsp->set_status_code(zhttp::HttpResponse::StatusCode::NotFound);
            rsp->set_status_message("Not Found");
            rsp->set_body("File not found");
            return;
        }

        FileUtil fu(info.real_path_);
        if (!fu.remove_file())
        {
            ZBACKUP_LOG_ERROR("Failed to delete file: {}", info.real_path_);
            rsp->set_status_code(zhttp::HttpResponse::StatusCode::InternalServerError);
            rsp->set_status_message("Internal Server Error");
            rsp->set_body("Failed to delete file");
            return;
        }

        if (info.pack_flag_)
        {
            FileUtil pack_fu(info.pack_path_);
            if (!pack_fu.remove_file())
            {
                ZBACKUP_LOG_WARN("Failed to delete pack file: {}", info.pack_path_);
            }
        }

        if (!data_manager_->delete_one(info))
        {
            ZBACKUP_LOG_ERROR("Failed to delete backup info from database: {}", file_url);
            rsp->set_status_code(zhttp::HttpResponse::StatusCode::InternalServerError);
            rsp->set_status_message("Internal Server Error");
            rsp->set_body("Failed to delete backup info");
            return;
        }

        if (!data_manager_->persistence())
        {
            ZBACKUP_LOG_ERROR("Failed to persist data after deletion: {}", file_url);
        }

        ZBACKUP_LOG_INFO("File deleted successfully: {}", info.real_path_);
        rsp->set_status_code(zhttp::HttpResponse::StatusCode::OK);
        rsp->set_status_message("OK");
        rsp->set_content_type("application/json");
        rsp->set_body(R"({"success": true, "message": "File deleted successfully"})");
    }
}

