#include <utility>
#include "interfaces/config_manager_interface.h"
#include "interfaces/storage_interface.h"
#include "interfaces/data_manager_interface.h"
#include "core/service_container.h"
#include "handlers/upload_handler.h"
#include "core/service_container.h"
#include "util/util.h"
#include <nlohmann/json.hpp>
#include "log/backup_logger.h"
#include <regex>
#include "interfaces/data_manager_interface.h"

namespace zbackup
{
    void UploadHandler::handle_request(const zhttp::HttpRequest &req, zhttp::HttpResponse *rsp)
    {
        std::string filename;
        std::string file_content;

        std::string content_type = req.get_header("Content-Type");
        ZBACKUP_LOG_DEBUG("Upload request received, Content-Type: {}", content_type);

        if (content_type.find("multipart/form-data") != std::string::npos)
        {
            if (!parse_multipart_data(req, filename, file_content))
            {
                ZBACKUP_LOG_WARN("Failed to parse multipart upload data");
                rsp->set_status_code(zhttp::HttpResponse::StatusCode::BadRequest);
                rsp->set_status_message("Bad Request");
                rsp->set_body("No file uploaded");
                return;
            }
        }
        else
        {
            filename = req.get_header("X-Filename");
            file_content = req.get_content();
            if (filename.empty())
            {
                ZBACKUP_LOG_WARN("Upload request missing X-Filename header");
                rsp->set_status_code(zhttp::HttpResponse::StatusCode::BadRequest);
                rsp->set_status_message("Bad Request");
                rsp->set_body("Missing X-Filename header");
                return;
            }
        }

        ZBACKUP_LOG_INFO("File upload started: {} ({} bytes)", filename, file_content.size());

        if (!save_file(filename, file_content))
        {
            ZBACKUP_LOG_ERROR("Failed to save uploaded file: {}", filename);
            rsp->set_status_code(zhttp::HttpResponse::StatusCode::InternalServerError);
            rsp->set_status_message("Internal Server Error");
            rsp->set_body("Write file failed");
            return;
        }

        ZBACKUP_LOG_INFO("File uploaded successfully: {}", filename);
        rsp->set_status_code(zhttp::HttpResponse::StatusCode::OK);
        rsp->set_status_message("OK");
        rsp->set_body("The file was uploaded successfully");
    }

    bool UploadHandler::parse_multipart_data(const zhttp::HttpRequest &req, std::string &filename,
                                             std::string &file_content)
    {
        std::string content_type = req.get_header("Content-Type");

        std::smatch match;
        std::regex boundary_re("boundary=([^\r\n;]+)");
        if (!std::regex_search(content_type, match, boundary_re) || match.size() < 2)
        {
            ZBACKUP_LOG_WARN("Multipart boundary not found in Content-Type: {}", content_type);
            return false;
        }

        std::string boundary = "--" + match[1].str();
        const std::string &body = req.get_content();
        ZBACKUP_LOG_DEBUG("Parsing multipart data with boundary: {}", boundary);

        size_t pos = 0;
        while ((pos = body.find(boundary, pos)) != std::string::npos)
        {
            size_t part_start = pos + boundary.size();
            if (body.substr(part_start, 2) == "--")
                break;

            size_t header_end = body.find("\r\n\r\n", part_start);
            if (header_end == std::string::npos)
                break;

            std::string headers = body.substr(part_start, header_end - part_start);
            size_t content_start = header_end + 4;
            size_t next_boundary = body.find(boundary, content_start);
            if (next_boundary == std::string::npos)
                break;

            size_t content_len = next_boundary - content_start;
            std::string part_content = body.substr(content_start, content_len);

            // 查找文件字段
            std::smatch fname_match;
            std::regex fname_re("name=\"file\".*filename=\"([^\"]*)\"");
            if (std::regex_search(headers, fname_match, fname_re))
            {
                if (fname_match.size() > 1)
                {
                    filename = fname_match[1].str();
                }
                // 去除结尾换行
                while (!part_content.empty() && (part_content.back() == '\r' || part_content.back() == '\n'))
                    part_content.pop_back();
                file_content = part_content;
                return true;
            }
            pos = next_boundary;
        }
        return false;
    }

    bool UploadHandler::save_file(const std::string &filename, const std::string &file_content) const
    {
        auto &container = core::ServiceContainer::get_instance();
        auto config = container.resolve<interfaces::IConfigManager>();
        auto data_manager = container.resolve<interfaces::IDataManager>();

        if (!config || !data_manager)
        {
            ZBACKUP_LOG_ERROR("Required services not available for file save");
            return false;
        }

        std::string back_dir = config->get_string("back_dir", "./backup/");
        std::string real_path = back_dir + util::FileUtil(filename).get_name();
        util::FileUtil fu(real_path);

        if (fu.set_content(file_content) == false)
        {
            ZBACKUP_LOG_ERROR("Failed to write file content: {}", real_path);
            return false;
        }

        info::BackupInfo info;
        if (info.new_backup_info(real_path) == false)
        {
            ZBACKUP_LOG_ERROR("Failed to create backup info for: {}", real_path);
            return false;
        }

        if (data_manager->insert(info) == false)
        {
            ZBACKUP_LOG_ERROR("Failed to insert backup info for: {}", real_path);
            return false;
        }

        return true;
    }
}
