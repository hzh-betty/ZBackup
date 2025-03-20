#pragma once
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <iostream>
#include <fstream>
#include <vector>
#include <experimental/filesystem>
#include <string>
#include <sys/stat.h>
#include <snappy.h>
#include <jsoncpp/json/json.h>
#include "logger.hpp"
/*
    实用工具类的编写
        文件操作
        压缩与解压缩
        JSON序列化与反序列化
*/
namespace zbackup
{
    namespace fs = std::experimental::filesystem;
    class FileUtil
    {
    public:
        FileUtil(const std::string &pathname) : pathname_(pathname)
        {
        }

        // 删除文件
        bool removeFile()
        {
            if (exists() == false)
            {
                logger->debug("remove file[{}] not exits", pathname_);
                return true;
            }

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

            logger->debug("get file[{}] size success", pathname_);
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

            logger->debug("get file[{}] last modify time success", pathname_);
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

            logger->debug("get file[{}] last access time success", pathname_);
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

            logger->debug("get file[{}] name success", filename);
            return filename;
        }

        // 获取文件数据
        bool getPosLen(std::string *body, size_t pos, size_t len)
        {
            // 1. 判断是否合法
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

            logger->debug("read file[{}] content success", pathname_);
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
                logger->error("write open file[{}] failed", pathname_);
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
            logger->debug("write file[{}] content success", pathname_);
            ofs.close();
            return true;
        }

        // 压缩文件（使用Snappy实现）
        bool compress(const std::string &packname)
        {
            std::string body;
            if (!getContent(&body)) // 获取原始文件数据
            {
                logger->warn("compress get file[{}] content failed", pathname_);
                return false;
            }
            logger->debug("compress get file[{}] content success", pathname_);

            // 1. 预分配
            std::string packed;
            packed.resize(body.size());

            // 2. 执行压缩
            if (!snappy::Compress(body.data(), body.size(), &packed))
            {
                logger->error("compress file[{}] failed", pathname_);
                return false;
            }
            logger->debug("compress file[{}] success", pathname_);

            // 3. 写入压缩文件
            FileUtil fu(packname);
            if (!fu.setContent(packed))
            {
                logger->error("compress write packed data to[{}] failed", packname);
                return false;
            }

            logger->debug("compress write packed data to[{}] success", packname);
            return true;
        }

        // 解压文件（使用Snappy实现）
        bool unCompress(const std::string &filename)
        {
            std::string body;
            if (!getContent(&body)) // 获取压缩文件数据
            {
                logger->warn("uncompress get file[{}] content failed", pathname_);
                return false;
            }
            logger->debug("uncompress get file[{}] content success", pathname_);

            // 1. 预分配解压缓冲区（需提前知道原始数据长度，此处假设压缩数据包含长度信息）
            size_t uncompressed_len = 0;
            if (!snappy::GetUncompressedLength(body.data(), body.size(), &uncompressed_len))
            {
                logger->error("uncompress get original length failed");
                return false;
            }
            logger->debug("uncompress get original length success");

            std::string unpacked;
            unpacked.resize(uncompressed_len);

            // 2. 执行解压
            if (!snappy::Uncompress(body.data(), body.size(), &unpacked))
            {
                logger->error("uncompress file[{}] failed", filename);
                return false;
            }
            logger->debug("uncompress file[{}] success", filename);

            // 3. 写入解压文件
            FileUtil fu(filename);
            if (!fu.setContent(unpacked))
            {
                logger->error("uncompress write unpacked data to[{}] failed", filename);
                return false;
            }
            logger->debug("uncompress write unpacked data to[{}] success", filename);
            return true;
        }

        bool exists()
        {
            return fs::exists(pathname_);
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
                logger->debug("create directories[{}] success", pathname_);
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
            logger->debug("all directories has be scanned");
        }

    private:
        std::string pathname_;
    };

    class JsonUtil
    {
    public:
        static bool Serialize(const Json::Value &root, std::string *str)
        {
            // 1. 创建一个建造类
            Json::StreamWriterBuilder swb;

            // 2. 建造一个StreamWriter类
            std::unique_ptr<Json::StreamWriter> sw(swb.newStreamWriter());

            // 3.写入
            std::stringstream ss;
            if (sw->write(root, &ss) != 0)
            {
                logger->warn("Serialize write error, error message");
                return false;
            }

            *str = ss.str();
            logger->debug("serialize has done");
            return true;
        }

        static bool Deserialize(Json::Value *root, const std::string &str)
        {
            // 1. 创建一个建造类
            Json::CharReaderBuilder crb;

            // 2. 创建一个CharReader类
            std::unique_ptr<Json::CharReader> cr(crb.newCharReader());

            // 3.读取
            std::string err;
            size_t len = str.size();
            bool ret = cr->parse(str.c_str(), str.c_str() + len, root, &err);
            if (ret == false)
            {
                logger->warn("Deserialize parse error, error message: {}", err);
                return false;
            }
            logger->debug("deserialize has done");
            return true;
        }
    };
}; // namespace zbackup
