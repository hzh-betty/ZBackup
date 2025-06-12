#include "../../include/core/config_service.h"
#include "../../include/util/util.h"
#include "../../include/log/logger.h"

namespace zbackup::core
{
    ConfigService::ConfigService(const std::string &config_file)
        : config_file_(config_file)
    {
        load_config();
        create_directories();
    }

    bool ConfigService::load_config()
    {
        FileUtil fu(config_file_);
        std::string body;

        if (!fu.exists())
        {
            ZBACKUP_LOG_ERROR("Config file not found: {}", config_file_);
            return false;
        }

        if (!fu.get_content(&body))
        {
            ZBACKUP_LOG_ERROR("Failed to read config file: {}", config_file_);
            return false;
        }

        if (!JsonUtil::deserialize(&config_json_, body))
        {
            ZBACKUP_LOG_ERROR("Failed to parse config file: {}", config_file_);
            return false;
        }

        ZBACKUP_LOG_INFO("Configuration loaded from: {}", config_file_);
        log_config_summary();
        return true;
    }

    void ConfigService::create_directories()
    {
        // 创建备份目录和压缩目录
        std::string back_dir = get_string("back_dir", "./backup/");
        std::string pack_dir = get_string("pack_dir", "./pack/");

        FileUtil back_util(back_dir);
        FileUtil pack_util(pack_dir);

        if (!back_util.create_directory())
        {
            ZBACKUP_LOG_ERROR("Failed to create backup directory: {}", back_dir);
        }
        if (!pack_util.create_directory())
        {
            ZBACKUP_LOG_ERROR("Failed to create pack directory: {}", pack_dir);
        }
    }

    void ConfigService::log_config_summary()
    {
        ZBACKUP_LOG_INFO("Server config - {}:{}", get_ip(), get_port());
        ZBACKUP_LOG_INFO("Directories - backup: {}, pack: {}",
                         get_string("back_dir", "./backup/"),
                         get_string("pack_dir", "./pack/"));
        ZBACKUP_LOG_INFO("SSL enabled: {}", get_bool("use_ssl", false));
        ZBACKUP_LOG_INFO("MySQL - {}:{}@{}/{}",
                         get_string("mysql_user", "root"),
                         get_int("mysql_port", 3306),
                         get_string("mysql_host", "localhost"),
                         get_string("mysql_db", "zbackup"));
    }

    std::string ConfigService::get_string(const std::string &key, const std::string &default_value)
    {
        return config_json_.value(key, default_value);
    }

    int ConfigService::get_int(const std::string &key, int default_value)
    {
        return config_json_.value(key, default_value);
    }

    bool ConfigService::get_bool(const std::string &key, bool default_value)
    {
        return config_json_.value(key, default_value);
    }

    uint16_t ConfigService::get_port() const
    {
        return static_cast<uint16_t>(config_json_.value("server_port", 8080));
    }

    std::string ConfigService::get_ip() const
    {
        return config_json_.value("server_ip", "0.0.0.0");
    }

    std::string ConfigService::get_download_prefix() const
    {
        return config_json_.value("download_prefix", "/download/");
    }

    // 新增的配置获取方法
    int ConfigService::get_hot_time() const
    {
        return config_json_.value("hot_time", 300);
    }

    std::string ConfigService::get_pack_file_prefix() const
    {
        return config_json_.value("packfile_suffix", ".pack");
    }

    std::string ConfigService::get_pack_dir() const
    {
        return config_json_.value("pack_dir", "./pack/");
    }

    std::string ConfigService::get_back_dir() const
    {
        return config_json_.value("back_dir", "./backup/");
    }

    std::string ConfigService::get_backup_file() const
    {
        return config_json_.value("backup_file", "./data/backup.dat");
    }

    bool ConfigService::get_use_ssl() const
    {
        return config_json_.value("use_ssl", false);
    }

    std::string ConfigService::get_cert_file_path() const
    {
        return config_json_.value("cert_file_path", "");
    }

    std::string ConfigService::get_key_file_path() const
    {
        return config_json_.value("key_file_path", "");
    }

    // MySQL 配置
    std::string ConfigService::get_mysql_host() const
    {
        return config_json_.value("mysql_host", "localhost");
    }

    uint16_t ConfigService::get_mysql_port() const
    {
        return static_cast<uint16_t>(config_json_.value("mysql_port", 3306));
    }

    std::string ConfigService::get_mysql_user() const
    {
        return config_json_.value("mysql_user", "root");
    }

    std::string ConfigService::get_mysql_password() const
    {
        return config_json_.value("mysql_password", "");
    }

    std::string ConfigService::get_mysql_db() const
    {
        return config_json_.value("mysql_db", "zbackup");
    }

    int ConfigService::get_mysql_pool_size() const
    {
        return config_json_.value("mysql_pool_size", 10);
    }

    // Redis 配置
    std::string ConfigService::get_redis_host() const
    {
        return config_json_.value("redis_host", "127.0.0.1");
    }

    int ConfigService::get_redis_port() const
    {
        return config_json_.value("redis_port", 6379);
    }

    std::string ConfigService::get_redis_password() const
    {
        return config_json_.value("redis_password", "");
    }

    int ConfigService::get_redis_db() const
    {
        return config_json_.value("redis_db", 0);
    }

    int ConfigService::get_redis_pool_size() const
    {
        return config_json_.value("redis_pool_size", 10);
    }

    int ConfigService::get_redis_timeout_ms() const
    {
        return config_json_.value("redis_timeout_ms", 5000);
    }
}
