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
        static DataManager* get_instance(Storage::ptr storage = nullptr);
        
        DataManager(const DataManager&) = delete;
        DataManager& operator=(const DataManager&) = delete;

        bool insert(const BackupInfo &info);
        bool update(const BackupInfo &info);
        bool get_one_by_url(const std::string &url, BackupInfo *info);
        bool get_one_by_real_path(const std::string &real_path, BackupInfo *info);
        void get_all(std::vector<BackupInfo> *arry);
        bool persistence();
        bool delete_one(const BackupInfo &info);
        bool delete_by_url(const std::string &url);
        bool delete_by_real_path(const std::string &real_path);

    private:
        explicit DataManager(Storage::ptr storage);

        mutable std::shared_mutex rw_mutex_;
        Storage::ptr storage_;
        
        static std::unique_ptr<DataManager> instance_;
        static std::once_flag init_flag_;
    };
}
