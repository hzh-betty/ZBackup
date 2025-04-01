#pragma once
#include "compress.hpp"
#include "data_manage.hpp"
#include "threadpool.hpp"
#include "../http/httplib.h"
namespace zbackup
{
    class BackupLooper
    {
    public:
        using ptr = std::shared_ptr<BackupLooper>;
        BackupLooper(Compress::ptr comp, Storage::ptr storage)
            : stop_(false), hotThread_(std::thread(&BackupLooper::hotMonitor, this)),
              dataManager_(std::make_shared<DataManager>(storage)), comp_(comp)
        {
        }

        void download(const httplib::Request &req, httplib::Response &rsp)
        {
            // 1. 根据资源路径，获取文件备份信息
            BackupInfo info;
            if (dataManager_->getOneByURL(req.path, &info) == false)
            {
                logger->warn("download get one by URL failed");
                return;
            }
            logger->debug("download get one by URL success");

            // 2. 判断文件是否被压缩，如果被压缩，要先解压缩,
            if (info.packFlag_ == true)
            {
                if (comp_->unCompress(info.realPath_, info.packPath_) == false)
                {
                    logger->warn("download unCompress failed");
                    return;
                }
                logger->debug("download unCompress success");

                // 3. 删除压缩包，修改备份信息（已经没有被压缩）
                FileUtil fu(info.packPath_);
                if (fu.removeFile() == false)
                {
                    logger->warn("download remove file[{}] failed", info.packPath_);
                    return;
                }
                info.packFlag_ = false;
                if (dataManager_->update(info) == false)
                {
                    logger->warn("download update file[{}] failed", info.packPath_);
                    return;
                }
                if (dataManager_->persistence() == false)
                {
                    logger->warn("download persistence file[{}] failed", info.packPath_);
                    return;
                }
            }

            // 4. 判断是否需要断点续传
            bool retrans = false;
            std::string old_etag;
            if (req.has_header("If-Range"))
            {
                old_etag = req.get_header_value("If-Range");
                if (old_etag == GetETag(info))
                {
                    retrans = true;
                }
            }

            // 5. 读取文件数据，放入rsp.body中
            FileUtil fu(info.realPath_);
            fu.getContent(&rsp.body);
            rsp.set_header("Accept-Ranges", "bytes");
            rsp.set_header("ETag", GetETag(info));
            rsp.set_header("Content-Type", "application/octet-stream");
            if (retrans == false)
            {
                rsp.status = 200;
                logger->info("download success");
            }
            else // httlplib支持断点续传
            {
                // std::string range = req.get_header_val("Range"); bytes start-end
                // rsp.set_header("Content-Range", "bytes start-end/fsize");
                rsp.status = 206; // 区间请求响应的是206
                logger->info("download breakpoint resumption success");
            }
        }
        void listshow(const httplib::Request &req, httplib::Response &rsp)
        {
            // 1. 获取所有的文件备份信息
            std::vector<BackupInfo> arry;
            dataManager_->getAll(&arry);
            logger->debug("listshow get all backiup info success");

            // 2. 根据所有备份信息，组织html文件数据
            std::stringstream ss;
            ss << "<html><head><title>Download</title></head>";
            ss << "<body><h1>Download</h1><table>";
            for (auto &a : arry)
            {
                ss << "<tr>";
                std::string filename = FileUtil(a.realPath_).getName();
                ss << "<td><a href='" << a.url_ << "'>" << filename << "</a></td>";
                ss << "<td align='right'>" << TimetoStr(a.mtime_) << "</td>";
                ss << "<td align='right'>" << a.fsize_ / 1024 << "k</td>";
                ss << "</tr>";
            }
            ss << "</table></body></html>";
            rsp.body = ss.str();
            rsp.set_header("Content-Type", "text/html; charset=UTF-8");
            rsp.status = 200;
            logger->debug("listshow organise html file dataManager success");

            logger->info("listshow success");
        }

        // post /upload 文件上传
        void upload(const httplib::Request &req, httplib::Response &rsp)
        {
            // 1. 读取文件内容
            bool ret = req.has_file("file");
            if (ret == false)
            {
                logger->warn("There is no such file field");
                rsp.status = 400;
                return;
            }
            logger->debug("This file field exists");
            const auto &file = req.get_file_value("file");

            // 2. 设置文件内容
            std::string backDir = Config::getInstance().getBackDir();
            std::string realPath = backDir + FileUtil(file.filename).getName();
            FileUtil fu(realPath);

            if (fu.setContent(file.content) == false)
            {
                logger->warn("upload set file contents failed");
                return;
            }
            logger->debug("upload set file contents success");

            // 3. 插入新的文件管理信息
            BackupInfo info;
            if (info.newBackupInfo(realPath) == false)
            {
                logger->warn("upload new backupInfo failed");
                return;
            }
            logger->debug("upload new backupInfo success");
            if (dataManager_->insert(info) == false)
            {
                logger->warn("upload insert backupInfo failed");
                return;
            }
            logger->debug("upload insert backupInfo success");
            if (dataManager_->persistence() == false)
            {
                logger->warn("upload persistence file[{}] failed", info.packPath_);
                return;
            }
            logger->info("The file was uploaded successfully");
        }

        ~BackupLooper()
        {
            stop_ = true;
            hotThread_.join();
        }

    private:
        void hotMonitor()
        {
            Config &config = Config::getInstance();
            std::string backDir = config.getBackDir();
            int hotTime = config.getHotTime();
            while (!stop_)
            {
                // 1. 遍历备份目录，获取所有文件名
                FileUtil fu(backDir);
                std::vector<std::string> arry;
                fu.scanDirectory(&arry);
                logger->debug("hotMonitor scan directory succsess");

                // 2. 判断是否为热点文件
                for (auto &str : arry)
                {
                    if (hotJudge(str, hotTime) == false)
                        continue;
                    ThreadPool::getInstance()->submitTask([&]()
                                                          { this->dealTask(str); });
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }
        }
        void dealTask(const std::string &str)
        {
            // 3. 获取文件信息
            BackupInfo bi;
            if (dataManager_->getOneByRealPath(str, &bi) == false)
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
            if (dataManager_->update(bi) == false)
            {
                logger->warn("hotMonitor update file[{}] failed", bi.packPath_);
                return;
            }
            if (dataManager_->persistence() == false)
            {
                logger->warn("upload persistence file[{}] failed", bi.packPath_);
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

        static std::string TimetoStr(time_t t)
        {
            std::string tmp = std::ctime(&t);
            logger->debug("time to str {}", tmp);
            return tmp;
        }

        static std::string GetETag(const BackupInfo &info)
        {
            // etg :  filename-fsize-mtime
            FileUtil fu(info.realPath_);
            std::string etag = fu.getName();
            etag += "-";
            etag += std::to_string(info.fsize_);
            etag += "-";
            etag += std::to_string(info.mtime_);
            logger->debug("create ETag {}", etag);
            return etag;
        }

    private:
        std::atomic<bool> stop_;
        std::thread hotThread_;
        DataManager::ptr dataManager_;
        Compress::ptr comp_;
    };
};
