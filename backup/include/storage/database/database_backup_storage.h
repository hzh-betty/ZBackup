#pragma once
#include "interfaces/backup_storage_interface.h"
#include "info/backup_info.h"
#include "db_pool/mysql_pool.h"

namespace zbackup::storage
{
    // 数据库备份存储实现类
    class DatabaseBackupStorage : public zbackup::interfaces::IBackupStorage
    {
    public:
        DatabaseBackupStorage();

        // 实现泛型存储接口
        bool insert(const info::BackupInfo &info) override;
        bool update(const info::BackupInfo &info) override;
        bool get_one_by_id(const std::string &id, info::BackupInfo *info) override;
        void get_all(std::vector<info::BackupInfo> *arry) override;
        bool delete_one(const info::BackupInfo &info) override;
        bool delete_by_id(const std::string &id) override;
        std::vector<info::BackupInfo> find_by_condition(const std::function<bool(const info::BackupInfo&)>& condition) override;

        // 实现备份存储特有接口
        bool get_one_by_url(const std::string &url, info::BackupInfo *info) override;
        bool get_one_by_real_path(const std::string &real_path, info::BackupInfo *info) override;
        bool delete_by_url(const std::string &url) override;
        bool delete_by_real_path(const std::string &real_path) override;

    private:
        bool create_table_if_not_exists();
    };
}
