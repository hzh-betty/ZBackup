#pragma once
#include "base_handler.hpp"

namespace zbackup
{
    class DeleteHandler : public BaseHandler
    {
    public:
        explicit DeleteHandler(Compress::ptr comp) : BaseHandler(comp) {}

        void handle_request(const zhttp::HttpRequest &req, zhttp::HttpResponse *rsp) override
        {
            // 1. 获取要删除的文件URL
            std::string file_url = req.get_query_parameters("url");
            if (file_url.empty())
            {
                logger->warn("delete: missing url parameter");
                rsp->set_status_code(zhttp::HttpResponse::StatusCode::BadRequest);
                rsp->set_status_message("Bad Request");
                rsp->set_body("Missing url parameter");
                return;
            }

            logger->info("delete: attempting to delete file with url [{}]", file_url);

            // 2. 根据URL获取备份信息
            BackupInfo info;
            if (!dataManager_->getOneByURL(file_url, &info))
            {
                logger->warn("delete: file not found for url [{}]", file_url);
                rsp->set_status_code(zhttp::HttpResponse::StatusCode::NotFound);
                rsp->set_status_message("Not Found");
                rsp->set_body("File not found");
                return;
            }

            // 3. 删除实际文件
            FileUtil fu(info.realPath_);
            if (!fu.removeFile())
            {
                logger->error("delete: failed to remove file [{}]", info.realPath_);
                rsp->set_status_code(zhttp::HttpResponse::StatusCode::InternalServerError);
                rsp->set_status_message("Internal Server Error");
                rsp->set_body("Failed to delete file");
                return;
            }

            // 4. 如果存在压缩文件，也删除
            if (info.packFlag_)
            {
                FileUtil pack_fu(info.packPath_);
                if (!pack_fu.removeFile())
                {
                    logger->warn("delete: failed to remove pack file [{}]", info.packPath_);
                }
            }

            // 5. 从数据管理中删除记录
            if (!dataManager_->deleteOne(info))
            {
                logger->error("delete: failed to delete backup info from database");
                rsp->set_status_code(zhttp::HttpResponse::StatusCode::InternalServerError);
                rsp->set_status_message("Internal Server Error");
                rsp->set_body("Failed to delete backup info");
                return;
            }

            // 6. 持久化数据
            if (!dataManager_->persistence())
            {
                logger->error("delete: failed to persist data");
                rsp->set_status_code(zhttp::HttpResponse::StatusCode::InternalServerError);
                rsp->set_status_message("Internal Server Error");
                rsp->set_body("Failed to persist data");
                return;
            }

            logger->info("delete: successfully deleted file [{}]", info.realPath_);
            rsp->set_status_code(zhttp::HttpResponse::StatusCode::OK);
            rsp->set_status_message("OK");
            rsp->set_content_type("application/json");
            rsp->set_body("{\"success\": true, \"message\": \"File deleted successfully\"}");
        }
    };
};
