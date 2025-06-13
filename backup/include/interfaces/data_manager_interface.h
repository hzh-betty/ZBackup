#pragma once
#include "../info/backup_info.h"
#include <vector>
#include <string>
#include <memory>

namespace zbackup::interfaces
{
    // 数据管理器接口
    class IDataManager
    {
    public:
        using ptr = std::shared_ptr<IDataManager>;
        virtual ~IDataManager() = default;

        // 基本操作
        virtual bool insert(const info::BackupInfo &info) = 0;
        virtual bool update(const info::BackupInfo &info) = 0;
        virtual bool get_one_by_url(const std::string &url, info::BackupInfo *info) = 0;
        virtual bool get_one_by_real_path(const std::string &real_path, info::BackupInfo *info) = 0;
        virtual void get_all(std::vector<info::BackupInfo> *arry) = 0;
        virtual bool delete_one(const info::BackupInfo &info) = 0;
        virtual bool delete_by_url(const std::string &url) = 0;
        virtual bool delete_by_real_path(const std::string &real_path) = 0;
        virtual bool persistence() = 0;
    };
}
