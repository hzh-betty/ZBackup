#pragma once
#include "base_handler.hpp"
#include <sstream>

namespace zbackup
{
    class ListShowHandler : public BaseHandler
    {
    public:
        explicit ListShowHandler(Compress::ptr comp) : BaseHandler(comp) {}

        void handle_request(const zhttp::HttpRequest &req, zhttp::HttpResponse *rsp) override
        {
            std::vector<BackupInfo> arry;
            dataManager_->getAll(&arry);
            logger->debug("listshow get all backup info success");

            std::stringstream ss;
            ss << "<html><head><title>Download</title></head>";
            ss << "<body><h1>Download</h1><table>";
            for (auto &a : arry)
            {
                ss << "<tr>";
                std::string filename = FileUtil(a.realPath_).getName();
                ss << "<td><a href='" << a.url_ << "'>" << filename << "</a></td>";
                ss << "<td align='right'>" << TimetoStr(a.mtime_) << "</td>";
                ss << "<td align='right'>" << a.fsize_ / 1024 << "k</td>";
                ss << "</tr>";
            }
            ss << "</table></body></html>";
            
            rsp->set_status_code(zhttp::HttpResponse::StatusCode::OK);
            rsp->set_status_message("OK");
            rsp->set_content_type("text/html; charset=UTF-8");
            rsp->set_body(ss.str());
        }
    };
};
