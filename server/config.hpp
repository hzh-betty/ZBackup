#pragma once
#include "util.hpp"
namespace zbackup
{
    static const std::string DEFAULT_CONFIG = "./backup.conf";
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
            Json::Value value;
            if (!JsonUtil::Deserialize(&value, body))
            {
                logger->fatal("deserialize config file[{}] failed", DEFAULT_CONFIG);
                return false;
            }
            logger->debug("deserialize config file[{}] success", DEFAULT_CONFIG);

            // 3.填写配置信息
            hotTime_ = value["hot_time"].asInt();
            serverPort_ = value["server_port"].asInt();
            serverip_ = value["server_ip"].asString();
            downloadPrefix_ = value["download_prefix"].asString();
            packfilePrefix_ = value["packfile_suffix"].asString();
            packDir_ = value["pack_dir"].asString();
            backDir_ = value["back_dir"].asString();
            backupFile_ = value["backup_file"].asString();
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
