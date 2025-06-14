#pragma once
#include "interfaces/compress_interface.h"
#include <snappy.h>

namespace zbackup
{
    // Snappy压缩算法实现类
    class SnappyCompress final :public interfaces::ICompress
    {
    public:
        bool compress(const std::string& source_path, const std::string& target_path) override; // 使用Snappy压缩
        bool un_compress(const std::string& target_path, const std::string& source_path) override; // 使用Snappy解压
    };
}
