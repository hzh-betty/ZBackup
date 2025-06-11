#pragma once
#include "../util/util.h"
#include "../storage/storage.h"
#include <shared_mutex>
#include <memory>
#include <mutex>

namespace zbackup
{
    class DataManager
    {
    public:
        static DataManager *get_instance();

        DataManager(const DataManager &) = delete;

        DataManager &operator=(const DataManager &) = delete;

        // 初始化存储后端
        void initialize_storage(Storage::ptr storage);

        bool insert(const BackupInfo &info) const;

        bool update(const BackupInfo &info) const;

        bool get_one_by_url(const std::string &url, BackupInfo *info) const;

        bool get_one_by_real_path(const std::string &real_path, BackupInfo *info) const;

        void get_all(std::vector<BackupInfo> *arry) const;

        bool persistence() const;

        bool delete_one(const BackupInfo &info) const;

        bool delete_by_url(const std::string &url) const;

        bool delete_by_real_path(const std::string &real_path) const;

    private:
        DataManager() = default;

        mutable std::shared_mutex rw_mutex_;
        Storage::ptr storage_;

        static std::unique_ptr<DataManager> instance_;
        static std::once_flag init_flag_;
    };
}
