#pragma once
#include "base_handler.hpp"
#include <regex>

namespace zbackup
{
    class UploadHandler : public BaseHandler
    {
    public:
        explicit UploadHandler(Compress::ptr comp) : BaseHandler(comp) {}

        void handle_request(const zhttp::HttpRequest &req, zhttp::HttpResponse *rsp) override
        {
            std::string filename;
            std::string filecontent;

            // 1. 检查是否为 multipart/form-data
            std::string content_type = req.get_header("Content-Type");
            logger->info("upload: Content-Type = [{}]", content_type);
            
            if (content_type.find("multipart/form-data") != std::string::npos)
            {
                if (!parseMultipartData(req, filename, filecontent))
                {
                    rsp->set_status_code(zhttp::HttpResponse::StatusCode::BadRequest);
                    rsp->set_status_message("Bad Request");
                    rsp->set_body("No file uploaded");
                    return;
                }
            }
            else
            {
                // 兼容原有X-Filename方式
                filename = req.get_header("X-Filename");
                filecontent = req.get_content();
                if (filename.empty())
                {
                    logger->warn("There is no X-Filename header");
                    rsp->set_status_code(zhttp::HttpResponse::StatusCode::BadRequest);
                    rsp->set_status_message("Bad Request");
                    rsp->set_body("Missing X-Filename header");
                    return;
                }
            }

            logger->debug("upload: filename={}, size={}", filename, filecontent.size());
            
            if (!saveFile(filename, filecontent))
            {
                rsp->set_status_code(zhttp::HttpResponse::StatusCode::InternalServerError);
                rsp->set_status_message("Internal Server Error");
                rsp->set_body("Write file failed");
                return;
            }

            rsp->set_status_code(zhttp::HttpResponse::StatusCode::OK);
            rsp->set_status_message("OK");
            rsp->set_body("The file was uploaded successfully");
        }

    private:
        // 解析 multipart/form-data 数据
        bool parseMultipartData(const zhttp::HttpRequest &req, std::string &filename, std::string &filecontent)
        {
            std::string content_type = req.get_header("Content-Type");
            
            // 提取 boundary
            std::smatch match;
            std::regex boundary_re("boundary=([^\r\n;]+)");
            if (!std::regex_search(content_type, match, boundary_re) || match.size() < 2)
            {
                logger->warn("multipart: boundary not found in Content-Type [{}]", content_type);
                return false;
            }
            
            std::string boundary = "--" + match[1].str();
            const std::string &body = req.get_content();
            
            size_t pos = 0;
            while ((pos = body.find(boundary, pos)) != std::string::npos)
            {
                size_t part_start = pos + boundary.size();
                if (body.substr(part_start, 2) == "--") break;

                size_t header_end = body.find("\r\n\r\n", part_start);
                if (header_end == std::string::npos) break;

                std::string headers = body.substr(part_start, header_end - part_start);
                size_t content_start = header_end + 4;
                size_t next_boundary = body.find(boundary, content_start);
                if (next_boundary == std::string::npos) break;
                
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
                    filecontent = part_content;
                    return true;
                }
                pos = next_boundary;
            }
            return false;
        }

        // 保存文件到指定路径
        bool saveFile(const std::string &filename, const std::string &filecontent)
        {
            std::string backDir = Config::getInstance().getBackDir();
            std::string realPath = backDir + FileUtil(filename).getName();
            FileUtil fu(realPath);

            if (fu.setContent(filecontent) == false)
            {
                logger->warn("upload set file contents failed");
                return false;
            }

            BackupInfo info;
            if (info.newBackupInfo(realPath) == false)
            {
                logger->warn("upload new backupInfo failed");
                return false;
            }
            
            if (dataManager_->insert(info) == false)
            {
                logger->warn("upload insert backupInfo failed");
                return false;
            }
            
            if (dataManager_->persistence() == false)
            {
                logger->warn("upload persistence failed");
                return false;
            }
            
            return true;
        }
    };
};
