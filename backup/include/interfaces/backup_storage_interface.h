#pragma once
#include "storage_interface.h"

// 前向声明
namespace zbackup::info
{
    class BackupInfo;
}

namespace zbackup::interfaces
{
    // 备份信息专用存储接口
    class IBackupStorage : public IStorage<info::BackupInfo>
    {
    public:
        using ptr = std::shared_ptr<IBackupStorage>;
        
        // 备份信息特有的查询方法
        virtual bool get_one_by_url(const std::string &url, info::BackupInfo *info) = 0;
        virtual bool get_one_by_real_path(const std::string &real_path, info::BackupInfo *info) = 0;
        virtual bool delete_by_url(const std::string &url) = 0;
        virtual bool delete_by_real_path(const std::string &real_path) = 0;
    };
}
