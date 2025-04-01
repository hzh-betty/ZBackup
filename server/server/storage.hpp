#pragma once
#include "util.hpp"
#include "config.hpp"
namespace zbackup
{
    struct BackupInfo
    {
        bool packFlag_; // 压缩标志
        size_t fsize_;
        time_t mtime_;
        time_t atime_;
        std::string realPath_;
        std::string packPath_;
        std::string url_;

        bool newBackupInfo(const std::string &realPath)
        {
            // 1. 获取文件信息
            FileUtil fu(realPath);
            if (fu.exists() == false)
            {
                logger->error("new backupInfo, file[{}] not exists", realPath);
                return false;
            }
            logger->debug("new backupInfo, file[{}] exists", realPath);

            std::string packdir = Config::getInstance().getPackDir();
            std::string packSuffix = Config::getInstance().getPackFilePrefix();
            std::string downStr = Config::getInstance().getDownloadPrefix();
            logger->info("get config file info success");

            // 2. 填写存储文件信息
            packFlag_ = false;
            fsize_ = fu.getSize();
            mtime_ = fu.getLastMTime();
            atime_ = fu.getLastATime();
            realPath_ = realPath;
            // ./backdir/a.txt -> ./packdir/a.txt.snp
            packPath_ = packdir + fu.getName() + packSuffix;
            // ./backdir/a.txt -> ./download/a.txt
            url_ = downStr + fu.getName();
            logger->info("fill in the bakeup info success");
            return true;
        }
    };

    class Storage
    {
    public:
        using ptr = std::shared_ptr<Storage>;
        // 添加备份信息
        virtual bool insert(const BackupInfo &info) = 0;
        // 更新备份信息
        virtual bool update(const BackupInfo &info) = 0;
        // 查找备份信息
        virtual bool getOneByURL(const std::string &url, BackupInfo *info) = 0;
        // 查找备份信息
        virtual bool getOneByRealPath(const std::string &realPath, BackupInfo *info) = 0;
        // 查找所有备份信息
        virtual void getAll(std::vector<BackupInfo> *arry) = 0;
        // 初始化加载
        virtual bool initLoad() = 0;
        // 持久化
        virtual bool persistence() = 0;
    };

    class FileStorage : public Storage
    {
    public:
        FileStorage()
        {
            backupFile_ = Config::getInstance().getBackupFile();
            initLoad();
        }

        // 添加备份信息
        bool insert(const BackupInfo &info) override
        {
            tables_[info.url_] = info;
            logger->debug("FileStorage insert info by url[{}] success", info.url_);
            return true;
        }

        // 更新备份信息
        bool update(const BackupInfo &info) override
        {
            tables_[info.url_] = info;
            logger->debug("FileStorage update info by url[{}] success", info.url_);
            return true;
        }

        // 查找备份信息
        bool getOneByURL(const std::string &url, BackupInfo *info) override
        {
            auto iter = tables_.find(url);
            if (iter == tables_.end())
            {
                logger->warn("FileStorage get info by url[{}] failed", url);
                return false;
            }
            *info = iter->second;
            logger->info("FileStorage get info by url[{}] success", url);
            return true;
        }

        bool getOneByRealPath(const std::string &realPath, BackupInfo *info) override
        {
            auto iter = tables_.begin();
            for (; iter != tables_.end(); ++iter)
            {
                if (iter->second.realPath_ == realPath)
                {
                    logger->info("FileStorage get info by realPathl[{}] success", realPath);
                    *info = iter->second;
                    return true;
                }
            }
            logger->warn("FileStorage get info by realPathl[{}] failed", realPath);
            return false;
        }

        void getAll(std::vector<BackupInfo> *arry) override
        {
            auto iter = tables_.begin();
            for (; iter != tables_.end(); ++iter)
            {
                arry->push_back(iter->second);
            }
            logger->info("FileStorage get all info success");
        }

    private:
        bool initLoad() override
        {
            // 1. 如果存储文件不存在就创建
            FileUtil fu(backupFile_);
            if (fu.exists() == false)
            {
                if (!fu.createFile())
                {
                    logger->fatal("FileStorage backup file[{}] cannot be created", backupFile_);
                    return false;
                }
                logger->debug("FileStorage backup file[{}] be created", backupFile_);
                return true;
            }

            // 2. 将数据文件中数据的读取
            std::string body;
            if (fu.getContent(&body) == false)
            {
                logger->fatal("FileStorage init load get file[{}] content failed", backupFile_);
                return false;
            }
            logger->debug("FileStorage init load get file[{}] content success", backupFile_);

            // 3.反序列化
            Json::Value root;
            if (JsonUtil::Deserialize(&root, body) == false)
            {
                logger->warn("FileStorage init load Deserialize failed");
                return false;
            }
            logger->debug("FileStorage init load Deserialize success");

            // 4.将反序列化好的数据添加进tables_
            for (int i = 0; i < root.size(); i++)
            {
                BackupInfo info;
                info.fsize_ = root[i]["fsize_"].asUInt64();
                info.atime_ = root[i]["atime_"].asUInt64();
                info.mtime_ = root[i]["mtime_"].asUInt64();
                info.packFlag_ = root[i]["packFlag_"].asBool();
                info.realPath_ = root[i]["realPath_"].asString();
                info.packPath_ = root[i]["packPath_"].asString();
                info.url_ = root[i]["url_"].asString();
                insert(info);
            }

            return true;
        }

        // 将信息持久化保存
        bool persistence() override
        {
            // 1. 获取所有数据
            std::vector<BackupInfo> array;
            getAll(&array);
            logger->debug("FileStorage persistence get all info success");

            // 2. 添加到Json::Value
            Json::Value root;
            for (int i = 0; i < array.size(); i++)
            {
                Json::Value item;
                item["packFlag_"] = array[i].packFlag_;
                item["fsize_"] = static_cast<Json::UInt64>(array[i].fsize_);
                item["atime_"] = static_cast<Json::UInt64>(array[i].atime_);
                item["mtime_"] = static_cast<Json::UInt64>(array[i].mtime_);
                item["realPath_"] = array[i].realPath_;
                item["packPath_"] = array[i].packPath_;
                item["url_"] = array[i].url_;
                root.append(item);
            }
            logger->debug("FileStorage persistence add all info success into json");

            // 3. 序列化
            std::string body;
            if (JsonUtil::Serialize(root, &body) == false)
            {
                logger->error("FileStorage serialize all persistence info failed");
                return false;
            }
            logger->debug("FileStorage serialize all persistence info success");

            // 4. 写文件
            FileUtil fu(backupFile_);
            if (fu.setContent(body) == false)
            {
                logger->error("FileStorage persistence info write to file[{}] failed ", backupFile_);
                return false;
            }
            logger->debug("FileStorage persistence info write to file[{}] success", backupFile_);

            logger->info("FileStorage the file was storaged successfully");
            return true;
        }

    protected:
        std::unordered_map<std::string, BackupInfo> tables_;
        std::string backupFile_;
    };
};