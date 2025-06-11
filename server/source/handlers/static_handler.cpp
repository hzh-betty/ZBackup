#include <utility>

#include "../../include/handlers/static_handler.h"

namespace zbackup
{
    StaticHandler::StaticHandler(Compress::ptr comp) : BaseHandler(std::move(comp))
    {
        // 设置MIME类型映射
        mime_types_[".html"] = "text/html; charset=UTF-8";
        mime_types_[".css"] = "text/css";
        mime_types_[".js"] = "application/javascript";
        mime_types_[".png"] = "image/png";
        mime_types_[".jpg"] = "image/jpeg";
        mime_types_[".jpeg"] = "image/jpeg";
        mime_types_[".gif"] = "image/gif";
        mime_types_[".ico"] = "image/x-icon";

        ZBACKUP_LOG_DEBUG("StaticHandler initialized with {} MIME types", mime_types_.size());
    }

    void StaticHandler::handle_request(const zhttp::HttpRequest &req, zhttp::HttpResponse *rsp)
    {
        std::string path = req.get_path();
        ZBACKUP_LOG_DEBUG("Static file request: {}", path);

        // 特殊处理根路径和index页面
        if (path == "/" || path == "/index" || path == "/index.html")
        {
            path = "/index.html";
        }

        // 移除前缀斜杠
        if (!path.empty() && path[0] == '/')
        {
            path = path.substr(1);
        }

        // 构建web根文件路径
        std::string file_path = "../resource/" + path;
        FileUtil fu(file_path);

        if (!fu.exists())
        {
            ZBACKUP_LOG_WARN("Static file not found: {}", file_path);
            rsp->set_status_code(zhttp::HttpResponse::StatusCode::NotFound);
            rsp->set_status_message("Not Found");
            rsp->set_body("File not found");
            return;
        }

        std::string content;
        if (!fu.get_content(&content))
        {
            ZBACKUP_LOG_ERROR("Failed to read static file: {}", file_path);
            rsp->set_status_code(zhttp::HttpResponse::StatusCode::InternalServerError);
            rsp->set_status_message("Internal Server Error");
            rsp->set_body("Failed to read file");
            return;
        }

        size_t dot_pos = path.find_last_of('.');
        std::string extension = (dot_pos != std::string::npos) ? path.substr(dot_pos) : "";
        std::string mime_type = get_mime_type(extension);

        rsp->set_status_code(zhttp::HttpResponse::StatusCode::OK);
        rsp->set_status_message("OK");
        rsp->set_content_type(mime_type);
        rsp->set_body(content);

        ZBACKUP_LOG_DEBUG("Static file served: {} ({} bytes, {})", file_path, content.size(), mime_type);
    }

    std::string StaticHandler::get_mime_type(const std::string &extension)
    {
        auto it = mime_types_.find(extension);
        return (it != mime_types_.end()) ? it->second : "application/octet-stream";
    }
}
