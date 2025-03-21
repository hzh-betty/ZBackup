#pragma once
#include <unistd.h>
#include <ctime>
#include "data_manage.hpp"

extern zbackup::DataManager *data_;

namespace zbackup
{
    class HotManager
    {
    public:
        HotManager()
        {
            // 1. 初始化
            Config &config = Config::getInstance();
            backDir_ = config.getBackDir();
            packDir_ = config.getPackDir();
            packSuffix_ = config.getPackFilePrefix();
            hotTime_ = config.getHotTime();
            logger->info("HotManager init success");
        }

        void runModule()
        {
            while (true)
            {
                // 1. 遍历备份目录，获取所有文件名
                FileUtil fu(backDir_);
                std::vector<std::string> arry;
                fu.scanDirectory(&arry);
                logger->debug("HotManager runModule scan directory succsess");

                // 2. 判断是否为热点文件
                for (auto &str : arry)
                {
                    if (hotJudge(str) == false)
                        continue;

                    // 3. 获取文件信息
                    BackupInfo bi;
                    if (data_->getOneByRealPath(str, &bi) == false)
                    {
                        logger->debug("there is a file[{}] present, but there is no backup information", str);
                        bi.newBackupInfo(str);
                    }

                    // 4. 对热点文件进行压缩
                    FileUtil tmp(str);
                    tmp.compress(bi.packPath_);

                    // 5. 删除源文件
                    tmp.removeFile();
                    bi.packFlag_ = true;
                    data_->update(bi);
                }

                usleep(10000);//避免空目录循环遍历，消耗cpu资源过高
            }
        }

    private:
        // 非热点文件-返回真；热点文件-返回假
        bool hotJudge(const std::string &filename)
        {
            FileUtil fu(filename);
            time_t lastAtime = fu.getLastATime();
            time_t curTime = time(nullptr);
            if (curTime - lastAtime > hotTime_)
            {
                logger->info("file[{}] is hot", filename);
                return true;
            }
            logger->info("file[{}] is not hot", filename);
            return false;
        }

    private:
        std::string backDir_;
        std::string packDir_;
        std::string packSuffix_;
        int hotTime_;
    };
};