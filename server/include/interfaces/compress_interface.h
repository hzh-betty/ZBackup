#pragma once
#include <string>
#include <memory>

namespace zbackup::interfaces
{
    // 压缩接口
    class ICompress
    {
    public:
        using ptr = std::shared_ptr<ICompress>;
        virtual ~ICompress() = default;

        // 压缩文件：将source_path文件压缩到target_path
        virtual bool compress(const std::string& source_path, const std::string& target_path) = 0;
        
        // 解压文件：将source_path压缩文件解压到target_path
        virtual bool un_compress(const std::string& target_path, const std::string& source_path) = 0;
    };
}
