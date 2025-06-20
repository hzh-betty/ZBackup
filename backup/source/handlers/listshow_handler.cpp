#include <utility>
#include "core/service_container.h"
#include "handlers/listshow_handler.h"
#include "util/util.h"
#include <nlohmann/json.hpp>
#include "log/backup_logger.h"
#include "interfaces/data_manager_interface.h"
#include "core/service_container.h"
namespace zbackup
{

    // 处理文件列表展示请求，生成HTML页面
    void ListShowHandler::handle_request(const zhttp::HttpRequest &req, zhttp::HttpResponse *rsp)
    {
        auto &container = core::ServiceContainer::get_instance();
        auto data_manager = container.resolve<interfaces::IDataManager>();

        if (!data_manager)
        {
            ZBACKUP_LOG_ERROR("DataManager not available for list display");
            rsp->set_status_code(zhttp::HttpResponse::StatusCode::InternalServerError);
            rsp->set_status_message("Internal Server Error");
            rsp->set_body("Service unavailable");
            return;
        }

        // 获取所有备份文件信息
        std::vector<info::BackupInfo> arry;
        data_manager->get_all(&arry);
        ZBACKUP_LOG_DEBUG("Retrieved {} backup entries for list display", arry.size());

        // 生成HTML表格展示文件列表
        std::stringstream ss;
        ss << "<html><head><title>Download</title></head>";
        ss << "<body><h1>Download</h1><table>";
        for (auto &a: arry)
        {
            ss << "<tr>";
            std::string filename = util::FileUtil(a.real_path_).get_name();
            ss << "<td><a href='" << a.url_ << "'>" << filename << "</a></td>";
            ss << "<td align='right'>" << util::time_to_str(a.mtime_) << "</td>";
            ss << "<td align='right'>" << a.fsize_ / 1024 << "k</td>";
            ss << "</tr>";
        }
        ss << "</table></body></html>";

        // 设置响应
        rsp->set_status_code(zhttp::HttpResponse::StatusCode::OK);
        rsp->set_status_message("OK");
        rsp->set_content_type("text/html; charset=UTF-8");
        rsp->set_body(ss.str());

        ZBACKUP_LOG_INFO("ListShowHandler processed request, generated HTML with {} entries", arry.size());
    }
}
