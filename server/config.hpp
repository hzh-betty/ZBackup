#pragma once
#include "util.hpp"
#include <filesystem>
namespace zbackup
{
    // 使用绝对路径，确保无论在哪启动都能找到
    static const std::string DEFAULT_CONFIG = "../config/config.json";
    class Config
    {
    public:
        static Config &getInstance()
        {
            static Config config;
            return config;
        }

        int getHotTime() const
        {
            return hotTime_;
        }

        uint16_t getPort() const
        {
            return serverPort_;
        }

        std::string getIp() const
        {
            return serverip_;
        }

        std::string getDownloadPrefix() const
        {
            return downloadPrefix_;
        }
        std::string getPackFilePrefix() const
        {
            return packfilePrefix_;
        }
        std::string getPackDir() const
        {
            return packDir_;
        }
        std::string getBackDir() const
        {
            return backDir_;
        }
        std::string getBackupFile() const
        {
            return backupFile_;
        }

    private:
        Config()
        {
            readConfigFile();
        }

        // 读取配置文件
        bool readConfigFile()
        {
            // 1. 打开文件
            FileUtil fu(DEFAULT_CONFIG);
            std::string body;
            if (fu.exists() == false)
            {
                logger->fatal("this config file[{}] not exists", DEFAULT_CONFIG);
                return false;
            }
            if (fu.getContent(&body) == false)
            {
                logger->fatal("load config file[{}] failed", DEFAULT_CONFIG);
                return false;
            }
            logger->debug("load config file[{}] success", DEFAULT_CONFIG);

            // 2. 反序列化
            nlohmann::json value;
            if (JsonUtil::Deserialize(&value, body) == false)
            {
                logger->fatal("deserialize config file[{}] failed", DEFAULT_CONFIG);
                return false;
            }
            logger->debug("deserialize config file[{}] success", DEFAULT_CONFIG);

            // 3.填写配置信息
            hotTime_ = value.value("hot_time", 0);
            serverPort_ = value.value("server_port", 0);
            serverip_ = value.value("server_ip", "");
            downloadPrefix_ = value.value("download_prefix", "");
            packfilePrefix_ = value.value("packfile_suffix", "");
            packDir_ = value.value("pack_dir", "");
            backDir_ = value.value("back_dir", "");
            backupFile_ = value.value("backup_file", "");
            logger->debug("load config file[{}] success", DEFAULT_CONFIG);

            // 4. 创建备份与压缩目录
            FileUtil tmp1(backDir_);
            FileUtil tmp2(packDir_);
            if (!tmp1.createDirectory())
            {
                logger->error("readConfigFile create backup dir[{}] failed", backDir_);
                return false;
            }
            if (!tmp2.createDirectory())
            {
                logger->error("readConfigFile create pack dir[{}] failed", backDir_);
                return false;
            }

            logger->info("read config file success", DEFAULT_CONFIG);
            return true;
        }

    private:
        int hotTime_; // 热点时间
        uint16_t serverPort_;
        std::string serverip_;
        std::string downloadPrefix_; // 下载目录前缀
        std::string packfilePrefix_; // 压缩包后缀
        std::string packDir_;        // 压缩包的存储路径
        std::string backDir_;        // 备份文件的存储路径
        std::string backupFile_;     // 备份的文件信息
    };
};
