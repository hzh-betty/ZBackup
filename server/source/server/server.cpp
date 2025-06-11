#include "../../include/server/server.h"
#include "../../include/config/config.h"
#include "../../ZHttpServer/include/middleware/middleware.h"
#include "../../ZHttpServer/include/session/session_manager.h"
#include "../../ZHttpServer/include/db_pool/redis_pool.h"

namespace zbackup
{
    // 备份服务器构造函数，初始化所有组件
    BackupServer::BackupServer(const Compress::ptr &comp, const Storage::ptr &storage)
        : looper_(std::make_shared<BackupLooper>(comp))
    {
        // 初始化数据管理器单例
        DataManager::get_instance(storage);

        // 初始化Redis连接池
        zhttp::zdb::RedisConnectionPool::get_instance().init("127.0.0.1");

        // 初始化会话管理器，使用Redis存储
        zhttp::zsession::SessionManager::get_instance()
                .set_session_storage(zhttp::zsession::StorageFactory<zhttp::zsession::DbSessionStorage>::create());

        // 初始化用户管理器
        user_manager_ = std::make_shared<UserManager>();

        // 读取配置信息
        Config &config = Config::get_instance();
        server_port_ = config.get_port();
        server_ip_ = config.get_ip();
        download_prefix_ = config.get_download_prefix();

        // 创建HTTP服务器
        auto builder = std::make_unique<zhttp::HttpServerBuilder>();
        builder->build_cert_file_path(config.get_cert_file_path());
        builder->build_key_file_path(config.get_key_file_path());
        builder->build_use_ssl(config.get_use_ssl());
        builder->build_port(server_port_);
        builder->build_name("BackupServer");
        builder->build_middleware(
            zhttp::zmiddleware::MiddlewareFactory::create<AuthMiddleware>());
        builder->build_thread_num(4);
        server_ = builder->build();

        // 创建各种请求处理器
        upload_handler_ = std::make_shared<UploadHandler>(comp);
        list_show_handler_ = std::make_shared<ListShowHandler>(comp);
        download_handler_ = std::make_shared<DownloadHandler>(comp);
        delete_handler_ = std::make_shared<DeleteHandler>(comp);
        static_handler_ = std::make_shared<StaticHandler>(comp);

        // 创建认证相关处理器
        login_handler_ = std::make_shared<LoginHandler>(comp, user_manager_);
        register_handler_ = std::make_shared<RegisterHandler>(comp, user_manager_);
        logout_handler_ = std::make_shared<LogoutHandler>(comp);

        ZBACKUP_LOG_INFO("BackupServer initialized on {}:{}", server_ip_, server_port_);
    }

    // 启动服务器并注册路由
    void BackupServer::run()
    {
        // 注册首页路由（无需认证）
        server_->Get("/home.html", static_handler_);
        server_->Get("/home", static_handler_);

        // 注册状态检查API
        server_->Get("/api/status", [](const zhttp::HttpRequest &req, zhttp::HttpResponse *rsp)
        {
            nlohmann::json status_json;
            try
            {
                auto session = zhttp::zsession::SessionManager::get_instance().get_session(req, rsp);
                auto logged_in = session->get_attribute("logged_in");
                status_json["logged_in"] = (logged_in == "true");
                if (logged_in == "true")
                {
                    status_json["username"] = session->get_attribute("username");
                }
            }
            catch (...)
            {
                status_json["logged_in"] = false;
            }

            std::string response_body;
            zbackup::JsonUtil::serialize(status_json, &response_body);

            rsp->set_status_code(zhttp::HttpResponse::StatusCode::OK);
            rsp->set_status_message("OK");
            rsp->set_content_type("application/json");
            rsp->set_body(response_body);
        });

        // 注册无需认证的路由（登录页面、注册页面等）
        server_->Get("/login.html", static_handler_);
        server_->Get("/register.html", static_handler_);
        server_->Post("/login", login_handler_);
        server_->Post("/register", register_handler_);

        // 默认根路径重定向到首页
        server_->Get("/", [](const zhttp::HttpRequest &req, zhttp::HttpResponse *rsp)
        {
            // 检查是否已登录
            try
            {
                auto session = zhttp::zsession::SessionManager::get_instance().get_session(req, rsp);
                auto logged_in = session->get_attribute("logged_in");
                if (logged_in == "true")
                {
                    // 已登录，重定向到主页面
                    rsp->set_status_code(zhttp::HttpResponse::StatusCode::Found);
                    rsp->set_status_message("Found");
                    rsp->set_header("Location", "/index.html");
                    return;
                }
            }
            catch (...)
            {
                // 会话检查失败，继续显示首页
            }

            // 未登录，重定向到首页
            rsp->set_status_code(zhttp::HttpResponse::StatusCode::Found);
            rsp->set_status_message("Found");
            rsp->set_header("Location", "/home.html");
        });

        // 注册需要认证的路由，所有业务功能都需要先登录
        server_->Get("/index.html", static_handler_);
        server_->Get("/index", static_handler_);
        server_->Post("/upload", upload_handler_);
        server_->Get("/listshow", list_show_handler_);
        server_->Delete("/delete", delete_handler_);
        server_->Post("/logout", logout_handler_);

        // 注册下载路由（需要认证）
        std::string download_url = download_prefix_ + "(.*)";
        server_->add_regex_route(zhttp::HttpRequest::Method::GET, download_url, download_handler_);

        ZBACKUP_LOG_INFO("BackupServer starting on port {}", server_port_);
        zbackup::ThreadPool::get_instance()->start(); // 启动线程池
        looper_->start(); // 启动后台热点监视
        server_->start();
    }
}
