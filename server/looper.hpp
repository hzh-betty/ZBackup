#pragma once
#include "compress.hpp"
#include "data_manage.hpp"
#include "threadpool.hpp"
#include <atomic>

namespace zbackup
{
    class BackupLooper
    {
    public:
        using ptr = std::shared_ptr<BackupLooper>;
        
        BackupLooper(Compress::ptr comp)
            : stop_(false), comp_(comp)
        {
            ThreadPool::getInstance()->submitTask([this]()
                                                  { hotMonitor(); });
        }

        ~BackupLooper()
        {
            stop_ = true;
        }

    private:
        void hotMonitor()
        {
            Config &config = Config::getInstance();
            std::string backDir = config.getBackDir();
            int hotTime = config.getHotTime();
            DataManager* dataManager = DataManager::getInstance();
            
            while (!stop_)
            {
                // 1. 遍历备份目录，获取所有文件名
                FileUtil fu(backDir);
                std::vector<std::string> arry;
                fu.scanDirectory(&arry);

                // 2. 判断是否为热点文件
                for (auto &str : arry)
                {
                    if (hotJudge(str, hotTime) == false)
                        continue;
                    ThreadPool::getInstance()->submitTask([this, str]()
                                                          { this->dealTask(str); });
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }
        }

        void dealTask(const std::string &str)
        {
            DataManager* dataManager = DataManager::getInstance();
            
            // 3. 获取文件信息
            BackupInfo bi;
            if (dataManager->getOneByRealPath(str, &bi) == false)
            {
                logger->warn("there is a file[{}] present, but there is no backup information", str);
                bi.newBackupInfo(str);
            }

            // 4. 对热点文件进行压缩
            comp_->compress(str, bi.packPath_);
            FileUtil tmp(str);

            // 5. 删除源文件
            tmp.removeFile();
            bi.packFlag_ = true;
            if (dataManager->update(bi) == false)
            {
                logger->warn("hotMonitor update file[{}] failed", bi.packPath_);
                return;
            }
            if (dataManager->persistence() == false)
            {
                logger->warn("hotMonitor persistence file[{}] failed", bi.packPath_);
            }
        }

        // 非热点文件-返回真；热点文件-返回假
        bool hotJudge(const std::string &filename, int hotTime)
        {
            FileUtil fu(filename);
            time_t lastAtime = fu.getLastATime();
            time_t curTime = time(nullptr);
            if (curTime - lastAtime > hotTime)
            {
                logger->debug("file[{}] is hot", filename);
                return true;
            }
            logger->debug("file[{}] is not hot", filename);
            return false;
        }

    private:
        std::atomic<bool> stop_;
        Compress::ptr comp_;
    };
};
