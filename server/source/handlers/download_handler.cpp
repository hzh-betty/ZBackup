#include <utility>

#include "../../include/handlers/download_handler.h"
#include "../../include/storage/storage.h"
#include "../../include/compress/snappy_compress.h"


namespace zbackup
{
    // 下载处理器构造函数
    DownloadHandler::DownloadHandler(interfaces::ICompress::ptr comp) : CompressHandler(std::move(comp))
    {
    }

    // 处理文件下载请求，支持断点续传
    void DownloadHandler::handle_request(const zhttp::HttpRequest &req, zhttp::HttpResponse *rsp)
    {
        std::string url_path = req.get_path();
        ZBACKUP_LOG_DEBUG("Download request: {}", url_path);

        // 根据URL获取文件备份信息
        BackupInfo info;
        if (data_manager_->get_one_by_url(url_path, &info) == false)
        {
            ZBACKUP_LOG_WARN("File not found for download: {}", url_path);
            rsp->set_status_code(zhttp::HttpResponse::StatusCode::NotFound);
            rsp->set_status_message("Not Found");
            rsp->set_body("Not Found");
            return;
        }

        // 如果文件被压缩，先解压缩
        if (info.pack_flag_ == true)
        {
            ZBACKUP_LOG_INFO("Decompressing file for download: {}", info.real_path_);
            // 解压缩文件
            if (comp_->un_compress(info.real_path_, info.pack_path_) == false)
            {
                ZBACKUP_LOG_ERROR("Failed to decompress file: {}", info.pack_path_);
                rsp->set_status_code(zhttp::HttpResponse::StatusCode::InternalServerError);
                rsp->set_status_message("Internal Server Error");
                rsp->set_body("Uncompress failed");
                return;
            }

            // 删除压缩包并更新备份信息
            FileUtil fu_pack(info.pack_path_);
            if (fu_pack.remove_file() == false)
            {
                ZBACKUP_LOG_WARN("Failed to remove pack file after decompression: {}", info.pack_path_);
            }

            info.pack_flag_ = false;
            if (data_manager_->update(info) == false)
            {
                ZBACKUP_LOG_WARN("Failed to update backup info after decompression: {}", info.real_path_);
            }
            if (data_manager_->persistence() == false)
            {
                ZBACKUP_LOG_ERROR("Failed to persist backup info after decompression: {}", info.real_path_);
                rsp->set_status_code(zhttp::HttpResponse::StatusCode::InternalServerError);
                rsp->set_status_message("Internal Server Error");
                rsp->set_body("Persistence failed");
                return;
            }
        }

        // 检查是否需要断点续传
        FileUtil fu(info.real_path_);
        int64_t file_size = fu.get_size();
        std::string range_header = req.get_header("Range");
        std::string old_etag = req.get_header("If-Range");

        // 处理断点续传请求
        if (!range_header.empty() && range_header.find("bytes=") == 0 && old_etag == get_etag(info))
        {
            handle_range_request(req, rsp, info, fu, file_size, range_header);
            return;
        }

        // 返回完整文件内容
        handle_full_request(rsp, info, fu);
    }

    // 处理断点续传请求
    void DownloadHandler::handle_range_request(const zhttp::HttpRequest &req, zhttp::HttpResponse *rsp,
                                               const BackupInfo &info, FileUtil &fu, int64_t file_size,
                                               const std::string &range_header)
    {
        ZBACKUP_LOG_DEBUG("Range request: {}", range_header);

        // 解析 Range: bytes=start-end
        size_t eq_pos = range_header.find('=');
        size_t dash_pos = range_header.find('-');
        if (eq_pos == std::string::npos || dash_pos == std::string::npos || dash_pos <= eq_pos)
        {
            ZBACKUP_LOG_WARN("Invalid Range header format: {}", range_header);
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
            ZBACKUP_LOG_WARN("Failed to parse Range values: {}", range_header);
            rsp->set_status_code(zhttp::HttpResponse::StatusCode::BadRequest);
            rsp->set_status_message("Bad Request");
            rsp->set_body("Invalid Range values");
            return;
        }

        if (start > end || end >= static_cast<size_t>(file_size))
        {
            ZBACKUP_LOG_WARN("Range out of bounds: {}-{}, file size={}", start, end, file_size);
            rsp->set_status_code(zhttp::HttpResponse::StatusCode::RangeNotSatisfiable);
            rsp->set_status_message("Requested Range Not Satisfiable");
            rsp->set_header("Content-Range", "bytes */" + std::to_string(file_size));
            rsp->set_body("Range Not Satisfiable");
            return;
        }

        size_t len = end - start + 1;
        std::string file_content;
        if (!fu.get_pos_len(&file_content, start, len))
        {
            ZBACKUP_LOG_ERROR("Failed to read file range [{}-{}] for: {}", start, end, info.real_path_);
            rsp->set_status_code(zhttp::HttpResponse::StatusCode::InternalServerError);
            rsp->set_status_message("Internal Server Error");
            rsp->set_body("Read file failed");
            return;
        }

        rsp->set_status_code(zhttp::HttpResponse::StatusCode::PartialContent);
        rsp->set_status_message("Partial Content");
        rsp->set_content_type("application/octet-stream");
        rsp->set_body(file_content);
        rsp->set_header("Accept-Ranges", "bytes");
        rsp->set_header("ETag", get_etag(info));
        rsp->set_header("Content-Range",
                        "bytes " + std::to_string(start) + "-" + std::to_string(end) + "/" + std::to_string(file_size));
        rsp->set_content_length(len);
        ZBACKUP_LOG_INFO("Partial download: {} [{}-{}] of {} bytes", info.real_path_, start, end, file_size);
    }

    // 处理完整文件下载请求
    void DownloadHandler::handle_full_request(zhttp::HttpResponse *rsp, const BackupInfo &info, FileUtil &fu)
    {
        std::string file_content;
        if (!fu.get_content(&file_content))
        {
            ZBACKUP_LOG_ERROR("Failed to read file for download: {}", info.real_path_);
            rsp->set_status_code(zhttp::HttpResponse::StatusCode::InternalServerError);
            rsp->set_status_message("Internal Server Error");
            rsp->set_body("Read file failed");
            return;
        }

        rsp->set_status_code(zhttp::HttpResponse::StatusCode::OK);
        rsp->set_status_message("OK");
        rsp->set_content_type("application/octet-stream");
        rsp->set_body(file_content);
        rsp->set_header("Accept-Ranges", "bytes");
        rsp->set_header("ETag", get_etag(info));
        rsp->set_content_length(file_content.size());

        ZBACKUP_LOG_INFO("Full download completed: {} ({} bytes)", info.real_path_, file_content.size());
    }
}
