#include "../../include/compress/snappy_compress.h"

namespace zbackup
{
    bool SnappyCompress::compress(const std::string& source_path, const std::string& target_path)
    {
        FileUtil tu(source_path);
        std::string body;
        if (tu.get_content(&body) == false)
        {
            ZBACKUP_LOG_ERROR("Failed to read file for compression: {}", source_path);
            return false;
        }

        std::string packed;
        packed.resize(body.size());

        if (!snappy::Compress(body.data(), body.size(), &packed))
        {
            ZBACKUP_LOG_ERROR("Snappy compression failed for: {}", source_path);
            return false;
        }

        FileUtil fu(target_path);
        if (!fu.set_content(packed))
        {
            ZBACKUP_LOG_ERROR("Failed to write compressed data to: {}", target_path);
            return false;
        }

        ZBACKUP_LOG_INFO("File compressed: {} -> {} ({} -> {} bytes)",
                         source_path, target_path, body.size(), packed.size());
        return true;
    }

    bool SnappyCompress::un_compress(const std::string& target_path, const std::string& source_path)
    {
        FileUtil tu(source_path);
        std::string body;
        if (tu.get_content(&body) == false)
        {
            ZBACKUP_LOG_ERROR("Failed to read compressed file: {}", source_path);
            return false;
        }

        size_t uncompressed_len = 0;
        if (!snappy::GetUncompressedLength(body.data(), body.size(), &uncompressed_len))
        {
            ZBACKUP_LOG_ERROR("Failed to get uncompressed length for: {}", source_path);
            return false;
        }

        std::string unpacked;
        unpacked.resize(uncompressed_len);

        if (!snappy::Uncompress(body.data(), body.size(), &unpacked))
        {
            ZBACKUP_LOG_ERROR("Snappy decompression failed for: {}", source_path);
            return false;
        }

        FileUtil fu(target_path);
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
