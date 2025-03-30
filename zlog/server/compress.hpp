#pragma once
#include "logger.hpp"
#include "util.hpp"
#include <snappy.h>
namespace zbackup
{
    class Compress
    {
    public:
        using ptr = std::shared_ptr<Compress>;
        virtual bool compress(const std::string pathname, const std::string &packname) = 0;
        virtual bool unCompress(const std::string pathname, const std::string &packname) = 0;
    };

    // 使用snappy压缩数据，追求速度
    class SnappyCompress : public Compress
    {
    public:
        bool compress(const std::string pathname, const std::string &packname) override
        {
            FileUtil tu(pathname);
            std::string body;
            if (tu.getContent(&body) == false) // 获取原始文件数据
            {
                logger->warn("compress get file[{}] content failed", pathname);
                return false;
            }
            logger->debug("compress get file[{}] content success", pathname);

            // 1. 预分配
            std::string packed;
            packed.resize(body.size());

            // 2. 执行压缩
            if (!snappy::Compress(body.data(), body.size(), &packed))
            {
                logger->error("compress file[{}] failed", pathname);
                return false;
            }
            logger->debug("compress file[{}] success", pathname);

            // 3. 写入压缩文件
            FileUtil fu(packname);
            if (!fu.setContent(packed))
            {
                logger->error("compress write packed data to[{}] failed", packname);
                return false;
            }

            logger->info("compress write packed data to[{}] success", packname);
            return true;
        }

        // 解压文件（使用Snappy实现）
        bool unCompress(const std::string pathname, const std::string &packname) override
        {
            FileUtil tu(packname);
            std::string body;
            if (tu.getContent(&body) == false) // 获取压缩文件数据
            {
                logger->warn("uncompress get file[{}] content failed", packname);
                return false;
            }
            logger->debug("uncompress get file[{}] content success", packname);

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
                logger->error("uncompress file[{}] failed", packname);
                return false;
            }
            logger->debug("uncompress file[{}] success", packname);

            // 3. 写入解压文件
            FileUtil fu(pathname);
            if (!fu.setContent(unpacked))
            {
                logger->error("uncompress write unpacked data to[{}] failed", pathname);
                return false;
            }
            logger->info("uncompress write unpacked data to[{}] success", pathname);
            return true;
        }
    };
};