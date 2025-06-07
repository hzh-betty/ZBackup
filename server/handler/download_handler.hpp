#pragma once
#include "base_handler.hpp"

namespace zbackup
{
    class DownloadHandler : public BaseHandler
    {
    public:
        explicit DownloadHandler(Compress::ptr comp) : BaseHandler(comp) {}

        void handle_request(const zhttp::HttpRequest &req, zhttp::HttpResponse *rsp) override
        {
            // 1. 根据资源路径，获取文件备份信息
            BackupInfo info;
            if (dataManager_->getOneByURL(req.get_path(), &info) == false)
            {
                logger->warn("download get one by URL failed");
                rsp->set_status_code(zhttp::HttpResponse::StatusCode::NotFound);
                rsp->set_status_message("Not Found");
                rsp->set_body("Not Found");
                return;
            }
            logger->debug("download get one by URL success");

            // 2. 判断文件是否被压缩，如果被压缩，要先解压缩
            if (info.packFlag_ == true)
            {
                if (comp_->unCompress(info.realPath_, info.packPath_) == false)
                {
                    logger->warn("download unCompress failed");
                    rsp->set_status_code(zhttp::HttpResponse::StatusCode::InternalServerError);
                    rsp->set_status_message("Internal Server Error");
                    rsp->set_body("Uncompress failed");
                    return;
                }
                logger->debug("download unCompress success");

                // 3. 删除压缩包，修改备份信息（已经没有被压缩）
                FileUtil fu(info.packPath_);
                if (fu.removeFile() == false)
                {
                    logger->warn("download remove file[{}] failed", info.packPath_);
                    rsp->set_status_code(zhttp::HttpResponse::StatusCode::InternalServerError);
                    rsp->set_status_message("Internal Server Error");
                    rsp->set_body("Remove pack file failed");
                    return;
                }
                info.packFlag_ = false;
                if (dataManager_->update(info) == false)
                {
                    logger->warn("download update file[{}] failed", info.packPath_);
                    rsp->set_status_code(zhttp::HttpResponse::StatusCode::InternalServerError);
                    rsp->set_status_message("Internal Server Error");
                    rsp->set_body("Update info failed");
                    return;
                }
                if (dataManager_->persistence() == false)
                {
                    logger->warn("download persistence file[{}] failed", info.packPath_);
                    rsp->set_status_code(zhttp::HttpResponse::StatusCode::InternalServerError);
                    rsp->set_status_message("Internal Server Error");
                    rsp->set_body("Persistence failed");
                    return;
                }
            }

            // 4. 判断是否需要断点续传
            FileUtil fu(info.realPath_);
            int64_t file_size = fu.getSize();
            std::string range_header = req.get_header("Range");
            std::string old_etag = req.get_header("If-Range");

            if (!range_header.empty() && range_header.find("bytes=") == 0 && old_etag == GetETag(info))
            {
                handleRangeRequest(req, rsp, info, fu, file_size, range_header);
                return;
            }

            // 返回全部内容
            handleFullRequest(rsp, info, fu);
        }

    private:
        void handleRangeRequest(const zhttp::HttpRequest &req, zhttp::HttpResponse *rsp, 
                               const BackupInfo &info, FileUtil &fu, int64_t file_size, 
                               const std::string &range_header)
        {
            logger->debug("download: Range header detected: [{}]", range_header);
            
            // 解析 Range: bytes=start-end
            size_t eq_pos = range_header.find('=');
            size_t dash_pos = range_header.find('-');
            if (eq_pos == std::string::npos || dash_pos == std::string::npos || dash_pos <= eq_pos)
            {
                logger->warn("download: invalid Range header format");
                rsp->set_status_code(zhttp::HttpResponse::StatusCode::BadRequest);
                rsp->set_status_message("Bad Request");
                rsp->set_body("Invalid Range header");
                return;
            }

            std::string start_str = range_header.substr(eq_pos + 1, dash_pos - eq_pos - 1);
            std::string end_str = range_header.substr(dash_pos + 1);
            size_t start = 0, end = file_size - 1;
            
            try
            {
                start = std::stoull(start_str);
                if (!end_str.empty())
                    end = std::stoull(end_str);
            }
            catch (...)
            {
                logger->warn("download: failed to parse Range values");
                rsp->set_status_code(zhttp::HttpResponse::StatusCode::BadRequest);
                rsp->set_status_message("Bad Request");
                rsp->set_body("Invalid Range values");
                return;
            }

            if (start > end || end >= static_cast<size_t>(file_size))
            {
                logger->warn("download: Range out of bounds: {}-{}, file size={}", start, end, file_size);
                rsp->set_status_code(zhttp::HttpResponse::StatusCode::RangeNotSatisfiable);
                rsp->set_status_message("Requested Range Not Satisfiable");
                rsp->set_header("Content-Range", "bytes */" + std::to_string(file_size));
                rsp->set_body("Range Not Satisfiable");
                return;
            }

            size_t len = end - start + 1;
            std::string fileContent;
            if (!fu.getPosLen(&fileContent, start, len))
            {
                rsp->set_status_code(zhttp::HttpResponse::StatusCode::InternalServerError);
                rsp->set_status_message("Internal Server Error");
                rsp->set_body("Read file failed");
                return;
            }

            rsp->set_status_code(zhttp::HttpResponse::StatusCode::PartialContent);
            rsp->set_status_message("Partial Content");
            rsp->set_content_type("application/octet-stream");
            rsp->set_body(fileContent);
            rsp->set_header("Accept-Ranges", "bytes");
            rsp->set_header("ETag", GetETag(info));
            rsp->set_header("Content-Range",
                "bytes " + std::to_string(start) + "-" + std::to_string(end) + "/" + std::to_string(file_size));
            rsp->set_content_length(len);
            logger->info("download: partial content [{}-{}] sent, total size={}", start, end, file_size);
        }

        void handleFullRequest(zhttp::HttpResponse *rsp, const BackupInfo &info, FileUtil &fu)
        {
            std::string fileContent;
            if (!fu.getContent(&fileContent))
            {
                rsp->set_status_code(zhttp::HttpResponse::StatusCode::InternalServerError);
                rsp->set_status_message("Internal Server Error");
                rsp->set_body("Read file failed");
                return;
            }
            
            rsp->set_status_code(zhttp::HttpResponse::StatusCode::OK);
            rsp->set_status_message("OK");
            rsp->set_content_type("application/octet-stream");
            rsp->set_body(fileContent);
            rsp->set_header("Accept-Ranges", "bytes");
            rsp->set_header("ETag", GetETag(info));
            rsp->set_content_length(fileContent.size());
            logger->info("download: full content sent, size={}", fileContent.size());
        }
    };
};
