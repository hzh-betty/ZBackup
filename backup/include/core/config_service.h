#pragma once
#include "interfaces/config_manager_interface.h"
#include <nlohmann/json.hpp>

namespace zbackup::core
{
    class ConfigService : public interfaces::IConfigManager
    {
    public:
        explicit ConfigService(const std::string& config_file = "../config/config.json");
        ~ConfigService() override = default;

        // IConfigManager 接口实现
        bool load_config() override;
        std::string get_string(const std::string& key, const std::string& default_value = "") override;
        int get_int(const std::string& key, int default_value = 0) override;
        bool get_bool(const std::string& key, bool default_value = false) override;
        uint16_t get_port() const override;
        std::string get_ip() const override;
        std::string get_download_prefix() const override;

        // 兼容原 Config 类的方法
        int get_hot_time() const;
        std::string get_pack_file_prefix() const;
        std::string get_pack_dir() const;
        std::string get_back_dir() const;
        std::string get_backup_file() const;
        bool get_use_ssl() const;
        std::string get_cert_file_path() const;
        std::string get_key_file_path() const;

        // MySQL配置
        std::string get_mysql_host() const;
        uint16_t get_mysql_port() const;
        std::string get_mysql_user() const;
        std::string get_mysql_password() const;
        std::string get_mysql_db() const;
        int get_mysql_pool_size() const;

        // Redis配置
        std::string get_redis_host() const;
        int get_redis_port() const;
        std::string get_redis_password() const;
        int get_redis_db() const;
        int get_redis_pool_size() const;
        int get_redis_timeout_ms() const;

    private:
        void create_directories();
        void log_config_summary();

        std::string config_file_;
        nlohmann::json config_json_;
    };
}
