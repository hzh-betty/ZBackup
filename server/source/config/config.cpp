#include "../../include/config/config.h"

namespace zbackup
{
    // 获取配置管理器单例
    Config &Config::get_instance()
    {
        static Config config;
        return config;
    }

    // 构造函数，自动读取配置文件
    Config::Config()
    {
        read_config_file();
    }

    // 读取并解析配置文件
    bool Config::read_config_file()
    {
        FileUtil fu(DEFAULT_CONFIG);
        std::string body;
        if (fu.exists() == false)
        {
            ZBACKUP_LOG_FATAL("Config file not found: {}", DEFAULT_CONFIG);
            return false;
        }
        if (fu.get_content(&body) == false)
        {
            ZBACKUP_LOG_FATAL("Failed to read config file: {}", DEFAULT_CONFIG);
            return false;
        }

        nlohmann::json value;
        if (JsonUtil::deserialize(&value, body) == false)
        {
            ZBACKUP_LOG_FATAL("Failed to parse config file: {}", DEFAULT_CONFIG);
            return false;
        }

        hot_time_ = value.value("hot_time", 0);
        server_port_ = value.value("server_port", 0);
        server_ip_ = value.value("server_ip", "");
        download_prefix_ = value.value("download_prefix", "");
        packfile_prefix_ = value.value("packfile_suffix", "");
        pack_dir_ = value.value("pack_dir", "");
        back_dir_ = value.value("back_dir", "");
        backup_file_ = value.value("backup_file", "");
        use_ssl_ = value.value("use_ssl", false);
        cert_file_path_ = value.value("cert_file_path", "");
        key_file_path_ = value.value("key_file_path", "");

        FileUtil tmp1(back_dir_);
        FileUtil tmp2(pack_dir_);
        if (!tmp1.create_directory())
        {
            ZBACKUP_LOG_ERROR("Failed to create backup directory: {}", back_dir_);
            return false;
        }
        if (!tmp2.create_directory())
        {
            ZBACKUP_LOG_ERROR("Failed to create pack directory: {}", pack_dir_);
            return false;
        }

        ZBACKUP_LOG_INFO("Configuration loaded successfully from {}", DEFAULT_CONFIG);
        ZBACKUP_LOG_INFO("Server: {}:{}, Backup dir: {}, Pack dir: {}", server_ip_, server_port_, back_dir_, pack_dir_);
        return true;
    }

    // 各种配置参数的getter方法
    int Config::get_hot_time() const
    {
        return hot_time_;
    }

    uint16_t Config::get_port() const
    {
        return server_port_;
    }

    std::string Config::get_ip() const
    {
        return server_ip_;
    }

    std::string Config::get_download_prefix() const
    {
        return download_prefix_;
    }

    std::string Config::get_pack_file_prefix() const
    {
        return packfile_prefix_;
    }

    std::string Config::get_pack_dir() const
    {
        return pack_dir_;
    }

    std::string Config::get_back_dir() const
    {
        return back_dir_;
    }

    std::string Config::get_backup_file() const
    {
        return backup_file_;
    }

    bool zbackup::Config::get_use_ssl() const
    {
        return use_ssl_;
    }

    std::string zbackup::Config::get_cert_file_path() const
    {
        return cert_file_path_;
    }

    std::string zbackup::Config::get_key_file_path() const
    {
        return key_file_path_;
    }
} // namespace zbackup
