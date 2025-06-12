#pragma once
#include <string>
#include <vector>
#include <memory>

namespace zbackup::interfaces
{
    // 备份任务状态
    enum class BackupStatus
    {
        Pending,
        Running,
        Completed,
        Failed,
        Cancelled
    };

    // 备份任务信息
    struct BackupTask
    {
        std::string id;
        std::string source_path;
        std::string destination_path;
        BackupStatus status;
        std::string created_time;
        std::string updated_time;
        std::string error_message;
    };

    // 备份管理器接口
    class IBackupManager
    {
    public:
        using ptr = std::shared_ptr<IBackupManager>;
        virtual ~IBackupManager() = default;

        // 创建备份任务
        virtual std::string create_backup_task(const std::string& source_path, 
                                              const std::string& destination_path) = 0;
        
        // 启动备份任务
        virtual bool start_backup_task(const std::string& task_id) = 0;
        
        // 停止备份任务
        virtual bool stop_backup_task(const std::string& task_id) = 0;
        
        // 获取任务状态
        virtual BackupTask get_backup_task(const std::string& task_id) = 0;
        
        // 获取所有任务
        virtual std::vector<BackupTask> get_all_backup_tasks() = 0;
        
        // 删除备份任务
        virtual bool delete_backup_task(const std::string& task_id) = 0;
    };
}
