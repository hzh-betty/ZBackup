/**
 * @file snappy_compress.cpp
 * @brief Snappy压缩算法实现，提供文件压缩和解压缩功能
 */

#include "compress/snappy_compress.h"
#include "log/backup_logger.h"
#include "util/util.h"

namespace zbackup
{
    /**
     * @brief 压缩文件
     * @param source_path 源文件路径
     * @param target_path 目标压缩文件路径
     * @return 压缩成功返回true，失败返回false
     */
    bool SnappyCompress::compress(const std::string& source_path, const std::string& target_path)
    {
        // 1. 读取源文件内容
        util::FileUtil tu(source_path);
        std::string body;
        if (tu.get_content(&body) == false)
        {
            ZBACKUP_LOG_ERROR("Failed to read file for compression: {}", source_path);
            return false;
        }

        // 2. 执行Snappy压缩
        std::string packed;
        packed.resize(body.size());

        if (!snappy::Compress(body.data(), body.size(), &packed))
        {
            ZBACKUP_LOG_ERROR("Snappy compression failed for: {}", source_path);
            return false;
        }

        // 3. 写入压缩后的数据
        util::FileUtil fu(target_path);
        if (!fu.set_content(packed))
        {
            ZBACKUP_LOG_ERROR("Failed to write compressed data to: {}", target_path);
            return false;
        }

        ZBACKUP_LOG_INFO("File compressed: {} -> {} ({} -> {} bytes)",
                         source_path, target_path, body.size(), packed.size());
        return true;
    }

    /**
     * @brief 解压缩文件
     * @param target_path 目标解压文件路径
     * @param source_path 源压缩文件路径
     * @return 解压成功返回true，失败返回false
     */
    bool SnappyCompress::un_compress(const std::string& target_path, const std::string& source_path)
    {
        // 1. 读取压缩文件内容
        util::FileUtil tu(source_path);
        std::string body;
        if (tu.get_content(&body) == false)
        {
            ZBACKUP_LOG_ERROR("Failed to read compressed file: {}", source_path);
            return false;
        }

        // 2. 获取解压后的数据长度
        size_t uncompressed_len = 0;
        if (!snappy::GetUncompressedLength(body.data(), body.size(), &uncompressed_len))
        {
            ZBACKUP_LOG_ERROR("Failed to get uncompressed length for: {}", source_path);
            return false;
        }

        // 3. 执行Snappy解压缩
        std::string unpacked;
        unpacked.resize(uncompressed_len);

        if (!snappy::Uncompress(body.data(), body.size(), &unpacked))
        {
            ZBACKUP_LOG_ERROR("Snappy decompression failed for: {}", source_path);
            return false;
        }

        // 4. 写入解压后的数据
        util::FileUtil fu(target_path);
        if (!fu.set_content(unpacked))
        {
            ZBACKUP_LOG_ERROR("Failed to write decompressed data to: {}", target_path);
            return false;
        }

        ZBACKUP_LOG_INFO("File decompressed: {} -> {} ({} -> {} bytes)",
                         source_path, target_path, body.size(), unpacked.size());
        return true;
    }
}
