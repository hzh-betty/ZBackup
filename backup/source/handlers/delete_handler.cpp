#include <utility>
#include "handlers/delete_handler.h"
#include "util/util.h"
#include <nlohmann/json.hpp>
#include "log/backup_logger.h"
#include "interfaces/data_manager_interface.h"
#include "core/service_container.h"
namespace zbackup
{
    void DeleteHandler::handle_request(const zhttp::HttpRequest &req, zhttp::HttpResponse *rsp)
    {
        auto &container = core::ServiceContainer::get_instance();
        auto data_manager = container.resolve<interfaces::IDataManager>();

        if (!data_manager)
        {
            ZBACKUP_LOG_ERROR("DataManager not available for delete operation");
            rsp->set_status_code(zhttp::HttpResponse::StatusCode::InternalServerError);
            rsp->set_status_message("Internal Server Error");
            rsp->set_body("Service unavailable");
            return;
        }

        // 从查询参数中获取要删除的文件URL
        std::string file_url = req.get_query_parameters("file");
        if (file_url.empty()) {
            // 如果查询参数为空，尝试从路径中获取（兼容其他格式）
            file_url = req.get_path();
            if (file_url == "/delete") {
                ZBACKUP_LOG_WARN("Delete request missing file parameter");
                rsp->set_status_code(zhttp::HttpResponse::StatusCode::BadRequest);
                rsp->set_status_message("Bad Request");
                rsp->set_body("Missing file parameter");
                return;
            }
        }
        
        ZBACKUP_LOG_INFO("Delete request for file: {}", file_url);

        info::BackupInfo info;
        if (!data_manager->get_one_by_url(file_url, &info))
        {
            ZBACKUP_LOG_WARN("File not found for deletion: {}", file_url);
            rsp->set_status_code(zhttp::HttpResponse::StatusCode::NotFound);
            rsp->set_status_message("Not Found");
            rsp->set_body("File not found");
            return;
        }

        util::FileUtil fu(info.real_path_);
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
            util::FileUtil pack_fu(info.pack_path_);
            if (!pack_fu.remove_file())
            {
                ZBACKUP_LOG_WARN("Failed to delete pack file: {}", info.pack_path_);
            }
        }

        if (!data_manager->delete_one(info))
        {
            ZBACKUP_LOG_ERROR("Failed to delete backup info from database: {}", file_url);
            rsp->set_status_code(zhttp::HttpResponse::StatusCode::InternalServerError);
            rsp->set_status_message("Internal Server Error");
            rsp->set_body("Failed to delete backup info");
            return;
        }

        if (!data_manager->persistence())
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

