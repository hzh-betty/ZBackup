#pragma once
#include "interfaces/data_manager_interface.h"
#include "interfaces/backup_storage_interface.h"
#include <memory>

namespace zbackup
{
    class DataManager : public interfaces::IDataManager
    {
    public:
        explicit DataManager(interfaces::IBackupStorage::ptr storage);
        ~DataManager() override = default;

        // 禁用拷贝和赋值
        DataManager(const DataManager &) = delete;
        DataManager &operator=(const DataManager &) = delete;

        // IDataManager 接口实现
        bool insert(const info::BackupInfo &info) override;
        bool update(const info::BackupInfo &info) override;
        bool get_one_by_url(const std::string &url, info::BackupInfo *info) override;
        bool get_one_by_real_path(const std::string &real_path, info::BackupInfo *info) override;
        void get_all(std::vector<info::BackupInfo> *arry) override;
        bool delete_one(const info::BackupInfo &info) override;
        bool delete_by_url(const std::string &url) override;
        bool delete_by_real_path(const std::string &real_path) override;
        bool persistence() override;

    private:
        interfaces::IBackupStorage::ptr storage_;
    };
}
