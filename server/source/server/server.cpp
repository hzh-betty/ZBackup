#include "../../include/server/server.h"
#include "../../include/config/config.h"

namespace zbackup
{
    // 备份服务器构造函数，初始化所有组件
    BackupServer::BackupServer(Compress::ptr comp, Storage::ptr storage)
        : looper_(std::make_shared<BackupLooper>(comp))
    {
        // 初始化数据管理器单例
        DataManager::get_instance(storage);
        
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
        builder->build_thread_num(4);
        builder->build_middleware();
        server_ = builder->build(MiddleWareFactory::create<zbackup::AuthMiddleware>());

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
        auth_middleware_ = std::make_shared<AuthMiddleware>();
        
        ZBACKUP_LOG_INFO("BackupServer initialized on {}:{}", server_ip_, server_port_);
    }

    // 启动服务器并注册路由
    void BackupServer::run()
    {
        // 注册认证相关路由（无需认证）
        server_->Post("/login", login_handler_);
        server_->Post("/register", register_handler_);
        server_->Post("/logout", logout_handler_);
        
        // 注册需要认证的API路由，并添加认证中间件
        server_->Post("/upload", upload_handler_, {auth_middleware_});
        server_->Get("/listshow", list_show_handler_, {auth_middleware_});
        server_->Delete("/delete", delete_handler_, {auth_middleware_});
        
        // 注册下载路由
        std::string download_url = download_prefix_ + "(.*)";
        server_->add_regex_route(zhttp::HttpRequest::Method::GET, download_url, download_handler_);
        
        // 注册静态文件路由（需要在最后，避免覆盖API路由）
        server_->Get("/", static_handler_);
        server_->Get("/index", static_handler_);
        server_->Get("/index.html", static_handler_);
        server_->Get("/login.html", static_handler_);
        server_->Get("/register.html", static_handler_);

        ZBACKUP_LOG_INFO("BackupServer starting on port {}", server_port_);
        server_->start();
    }
}
