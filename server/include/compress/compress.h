#pragma once
#include "../log/logger.h"
#include "../util/util.h"
#include <snappy.h>

namespace zbackup
{
    // 压缩接口基类
    class Compress
    {
    public:
        using ptr = std::shared_ptr<Compress>;

        virtual ~Compress() = default;

        virtual bool compress(std::string pathname, const std::string &packname) = 0; // 压缩文件
        virtual bool un_compress(std::string pathname, const std::string &packname) = 0; // 解压文件
    };

    // Snappy压缩算法实现类
    class SnappyCompress final : public Compress
    {
    public:
        bool compress(std::string pathname, const std::string &packname) override; // 使用Snappy压缩
        bool un_compress(std::string pathname, const std::string &packname) override; // 使用Snappy解压
    };
}
