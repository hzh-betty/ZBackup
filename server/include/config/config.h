#pragma once
#include "../util/util.h"
#include <filesystem>

namespace zbackup
{
    static const std::string DEFAULT_CONFIG = "../config/config.json";

    class Config
    {
    public:
        static Config &get_instance();

        [[nodiscard]] int get_hot_time() const;

        [[nodiscard]] uint16_t get_port() const;

        [[nodiscard]] std::string get_ip() const;

        [[nodiscard]] std::string get_download_prefix() const;

        [[nodiscard]] std::string get_pack_file_prefix() const;

        [[nodiscard]] std::string get_pack_dir() const;

        [[nodiscard]] std::string get_back_dir() const;

        [[nodiscard]] std::string get_backup_file() const;

        [[nodiscard]] bool get_use_ssl() const;

        [[nodiscard]] std::string get_cert_file_path() const;

        [[nodiscard]] std::string get_key_file_path() const;

        // 新增数据库配置获取方法
        [[nodiscard]] std::string get_db_host() const;

        [[nodiscard]] uint16_t get_db_port() const;

        [[nodiscard]] std::string get_db_user() const;

        [[nodiscard]] std::string get_db_password() const;

        [[nodiscard]] std::string get_db_name() const;

    private:
        Config();

        bool read_config_file();

    private:
        int hot_time_{};
        uint16_t server_port_{};
        std::string server_ip_;
        std::string download_prefix_;
        std::string packfile_prefix_;
        std::string pack_dir_;
        std::string back_dir_;
        std::string backup_file_;
        bool use_ssl_ = false;
        std::string cert_file_path_;
        std::string key_file_path_;

        // 新增数据库配置成员变量
        std::string db_host_;
        uint16_t db_port_{};
        std::string db_user_;
        std::string db_password_;
        std::string db_name_;
    };
}
