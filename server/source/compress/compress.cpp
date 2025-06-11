#include "../../include/compress/compress.h"

namespace zbackup
{
    bool SnappyCompress::compress(const std::string pathname, const std::string &packname)
    {
        FileUtil tu(pathname);
        std::string body;
        if (tu.get_content(&body) == false)
        {
            ZBACKUP_LOG_ERROR("Failed to read file for compression: {}", pathname);
            return false;
        }

        std::string packed;
        packed.resize(body.size());

        if (!snappy::Compress(body.data(), body.size(), &packed))
        {
            ZBACKUP_LOG_ERROR("Snappy compression failed for: {}", pathname);
            return false;
        }

        FileUtil fu(packname);
        if (!fu.set_content(packed))
        {
            ZBACKUP_LOG_ERROR("Failed to write compressed data to: {}", packname);
            return false;
        }

        ZBACKUP_LOG_INFO("File compressed: {} -> {} ({} -> {} bytes)",
                         pathname, packname, body.size(), packed.size());
        return true;
    }

    bool SnappyCompress::un_compress(const std::string pathname, const std::string &packname)
    {
        FileUtil tu(packname);
        std::string body;
        if (tu.get_content(&body) == false)
        {
            ZBACKUP_LOG_ERROR("Failed to read compressed file: {}", packname);
            return false;
        }

        size_t uncompressed_len = 0;
        if (!snappy::GetUncompressedLength(body.data(), body.size(), &uncompressed_len))
        {
            ZBACKUP_LOG_ERROR("Failed to get uncompressed length for: {}", packname);
            return false;
        }

        std::string unpacked;
        unpacked.resize(uncompressed_len);

        if (!snappy::Uncompress(body.data(), body.size(), &unpacked))
        {
            ZBACKUP_LOG_ERROR("Snappy decompression failed for: {}", packname);
            return false;
        }

        FileUtil fu(pathname);
        if (!fu.set_content(unpacked))
        {
            ZBACKUP_LOG_ERROR("Failed to write decompressed data to: {}", pathname);
            return false;
        }

        ZBACKUP_LOG_INFO("File decompressed: {} -> {} ({} -> {} bytes)",
                         packname, pathname, body.size(), unpacked.size());
        return true;
    }
}
