#pragma once
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <fstream>
#include <filesystem>
#include <sys/stat.h>
#include <nlohmann/json.hpp> // 替换jsoncpp
#include "logger.hpp"

/*
    实用工具类的编写
        文件操作
        压缩与解压缩
        JSON序列化与反序列化
*/
namespace zbackup
{
    namespace fs = std::filesystem;
    class FileUtil
    {
    public:
        FileUtil(const std::string &pathname) : pathname_(pathname)
        {
        }

        // 删除文件
        bool removeFile()
        {
            // 1. 判断文件是否存在
            if (exists() == false)
            {
                logger->debug("remove file[{}] not exits", pathname_);
                return true;
            }

            // 2. 删除指定文件
            if (remove(pathname_.c_str()) == -1)
            {
                logger->error("remove file[{}] falied, erro info is {}", pathname_, strerror(errno));
                return false;
            }
            logger->info("remove file[{}] success", pathname_);
            return true;
        }

        // 获取文件大小
        int64_t getSize() const
        {
            struct stat st;
            if (stat(pathname_.c_str(), &st) < 0)
            {
                logger->warn("get file[{}] size failed", pathname_);
                return -1;
            }

            logger->info("get file[{}] size success", pathname_);
            return st.st_size;
        }

        // 获取文件上一次修改时间
        time_t getLastMTime() const
        {
            struct stat st;
            if (stat(pathname_.c_str(), &st) < 0)
            {
                logger->warn("get file[{}] last modify time failed", pathname_);
                return -1;
            }

            logger->info("get file[{}] last modify time success", pathname_);
            return st.st_mtime;
        }

        // 获取文件上一次访问时间
        time_t getLastATime() const
        {
            struct stat st;
            if (stat(pathname_.c_str(), &st) < 0)
            {
                logger->warn("get file[{}] last access time failed", pathname_);
                return -1;
            }

            logger->info("get file[{}] last access time success", pathname_);
            return st.st_atime;
        }

        // 获取文件名称
        // a/b/c/tmp.txt
        std::string getName() const
        {
            std::string filename;
            size_t pos = pathname_.find_last_of("/");
            if (pos == std::string::npos)
            {
                filename = pathname_;
            }
            else
            {
                filename = pathname_.substr(pos + 1);
            }

            logger->info("get file[{}] name success", filename);
            return filename;
        }

        // 获取文件数据
        bool getPosLen(std::string *body, size_t pos, size_t len)
        {
            // 1. 判断是否合法
            if(exists() == false)
            {
                logger->warn("this file[{}] not exists", pathname_);
                return false;
            }

            int64_t fsize = getSize();
            if (pos + len > fsize)
            {
                logger->warn("[{}] len is too long", pathname_);
                return false;
            }

            // 2. 打开文件
            std::ifstream ifs;
            ifs.open(pathname_, std::ios::in | std::ios::binary);
            if (!ifs.is_open())
            {
                logger->error("read open file[{}] failed", pathname_);
                return false;
            }
            logger->debug("read open file[{}] success", pathname_);

            // 3. 获取数据
            ifs.seekg(pos, std::ios::beg);
            body->resize(len);
            ifs.read(&(*body)[0], len);

            if (!ifs.good())
            {
                logger->warn("read file[{}] content failed", pathname_);
                ifs.close();
                return false;
            }

            logger->info("read file[{}] content success", pathname_);
            ifs.close();
            return true;
        }

        bool getContent(std::string *body)
        {
            int64_t fsize = getSize();
            return getPosLen(body, 0, fsize);
        }

        // 写入文件数据
        bool setContent(const std::string &body)
        {
            // 1. 打开文件
            std::ofstream ofs;
            ofs.open(pathname_, std::ios::out | std::ios::binary);
            if (!ofs.is_open())
            {
                logger->warn("write open file[{}] failed", pathname_);
                return false;
            }
            logger->debug("write open file[{}] success", pathname_);

            // 2. 写入数据
            ofs.write(&body[0], body.size());
            if (!ofs.good())
            {
                logger->warn("write file[{}] content failed", pathname_);
                ofs.close();
                return false;
            }

            logger->info("write file[{}] content success", pathname_);
            ofs.close();
            return true;
        }

        bool exists()
        {
            return fs::exists(pathname_);
        }
        
        bool createFile()
        {
            std::ofstream createFile(pathname_);
            if (createFile.is_open() == false)
            {
                logger->fatal("create file[{}] open failed", pathname_);
                return false;
            }
            createFile.close();
            logger->info("create file[{}] success", pathname_);
            return true;
        }
        // 创建目录
        bool createDirectory()
        {
            if (exists())
                return true;
            bool ret = fs::create_directories(pathname_);
            if (ret == false)
            {
                logger->error("create directories[{}] failed", pathname_);
            }
            else
            {
                logger->info("create directories[{}] success", pathname_);
            }
            return ret;
        }

        // 浏览目录
        void scanDirectory(std::vector<std::string> *arry)
        {
            for (auto &p : fs::directory_iterator(pathname_))
            {
                if (fs::is_directory(p))
                    continue;
                std::string dir = fs::path(p).relative_path().string();
                logger->debug("[{}] is be scanned", dir);
                arry->push_back(dir);
            }
            logger->info("all directories has be scanned");
        }

    private:
        std::string pathname_;
    };

    class JsonUtil
    {
    public:
        // 序列化
        static bool Serialize(const nlohmann::json &root, std::string *str)
        {
            try
            {
                *str = root.dump();
                logger->info("serialize has done");
                return true;
            }
            catch (const std::exception &e)
            {
                logger->warn("Serialize write error: {}", e.what());
                return false;
            }
        }

        // 反序列化
        static bool Deserialize(nlohmann::json *root, const std::string &str)
        {
            try
            {
                *root = nlohmann::json::parse(str);
                logger->info("deserialize has done");
                return true;
            }
            catch (const std::exception &e)
            {
                logger->warn("Deserialize parse error: {}", e.what());
                return false;
            }
        }
    };
}; // namespace zbackup
