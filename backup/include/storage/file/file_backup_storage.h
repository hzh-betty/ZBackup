#pragma once
#include "interfaces/backup_storage_interface.h"
#include "info/backup_info.h"
#include <unordered_map>
#include <mutex>

namespace zbackup::storage
{
    // 文件存储实现类 - 专门用于备份信息
    class FileBackupStorage : public zbackup::interfaces::IBackupStorage
    {
    public:
        FileBackupStorage();

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
        bool init_load(); // 从文件加载数据
        bool save_to_file(); // 保存数据到文件

        std::unordered_map<std::string, info::BackupInfo> tables_; // 内存存储表
        std::string backup_file_; // 备份文件路径
        mutable std::mutex file_mutex_; // 文件操作互斥锁
    };
}
