#pragma once
#include "interfaces/base_info_interface.h"
#include <ctime>

namespace zbackup::info
{
    // 备份文件信息类
    class BackupInfo : public interfaces::IBaseInfo
    {
    public:
        // 构造函数
        BackupInfo() = default;
        BackupInfo(const std::string& real_path);
        bool new_backup_info(const std::string &real_path);

        // 实现基础接口
        std::string get_id() const override { return url_; }
        void set_id(const std::string& id) override { url_ = id; }
        std::string serialize() const override;
        bool deserialize(const std::string& data) override;
        interfaces::IBaseInfo::ptr clone() const override;
          
        // 公共成员变量
        bool pack_flag_ = false; // 是否已压缩
        size_t fsize_ = 0; // 文件大小
        time_t mtime_ = 0; // 修改时间
        time_t atime_ = 0; // 访问时间
        std::string real_path_; // 真实文件路径
        std::string pack_path_; // 压缩文件路径
        std::string url_; // 下载URL (唯一标识符)
    };
}
