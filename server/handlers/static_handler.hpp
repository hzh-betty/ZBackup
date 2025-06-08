#pragma once
#include "base_handler.hpp"
#include <unordered_map>

namespace zbackup
{
    class StaticHandler : public BaseHandler
    {
    public:
        explicit StaticHandler(Compress::ptr comp) : BaseHandler(comp) 
        {
            // 设置MIME类型映射
            mimeTypes_[".html"] = "text/html; charset=UTF-8";
            mimeTypes_[".css"] = "text/css";
            mimeTypes_[".js"] = "application/javascript";
            mimeTypes_[".png"] = "image/png";
            mimeTypes_[".jpg"] = "image/jpeg";
            mimeTypes_[".jpeg"] = "image/jpeg";
            mimeTypes_[".gif"] = "image/gif";
            mimeTypes_[".ico"] = "image/x-icon";
        }

        void handle_request(const zhttp::HttpRequest &req, zhttp::HttpResponse *rsp) override
        {
            std::string path = req.get_path();
            
            // 如果是根路径，重定向到index.html
            if (path == "/" || path == "/index" || path == "/index.html") {
                path = "/index.html";
            }

            // 移除前缀斜杠
            if (!path.empty() && path[0] == '/') {
                path = path.substr(1);
            }

            // 构建web根文件路径
            std::string filePath = "../resource/" + path;
            FileUtil fu(filePath);

            if (!fu.exists()) {
                logger->warn("static: file not found [{}]", filePath);
                rsp->set_status_code(zhttp::HttpResponse::StatusCode::NotFound);
                rsp->set_status_message("Not Found");
                rsp->set_body("File not found");
                return;
            }

            std::string content;
            if (!fu.getContent(&content)) {
                logger->error("static: failed to read file [{}]", filePath);
                rsp->set_status_code(zhttp::HttpResponse::StatusCode::InternalServerError);
                rsp->set_status_message("Internal Server Error");
                rsp->set_body("Failed to read file");
                return;
            }

            // 设置MIME类型
            size_t dotPos = path.find_last_of('.');
            std::string extension = (dotPos != std::string::npos) ? path.substr(dotPos) : "";
            std::string mimeType = getMimeType(extension);

            rsp->set_status_code(zhttp::HttpResponse::StatusCode::OK);
            rsp->set_status_message("OK");
            rsp->set_content_type(mimeType);
            rsp->set_body(content);
            
            logger->debug("static: served file [{}] with type [{}]", filePath, mimeType);
        }

    private:
        std::string getMimeType(const std::string &extension) 
        {
            auto it = mimeTypes_.find(extension);
            return (it != mimeTypes_.end()) ? it->second : "application/octet-stream";
        }

        std::unordered_map<std::string, std::string> mimeTypes_;
    };
};
