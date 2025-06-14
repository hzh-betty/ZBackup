#include <utility>
#include "server/looper.h"
#include "core/service_container.h"
#include "core/threadpool.h"
#include "interfaces/config_manager_interface.h"
#include "util/util.h"
#include "data/data_manager.h"
#include "log/backup_logger.h"

namespace zbackup
{
    // 后台循环构造函数，启动热点文件监控
    BackupLooper::BackupLooper(interfaces::ICompress::ptr comp)
        : stop_(false), comp_(std::move(comp))
    {
        ZBACKUP_LOG_INFO("BackupLooper initialized");
    }

    void BackupLooper::start() const
    {
        core::ThreadPool::get_instance()->submit_task([this]()
        {
            hot_monitor();
        });
        ZBACKUP_LOG_INFO("BackupLooper started monitoring for hot files");
    }

    // 析构函数，停止监控循环
    BackupLooper::~BackupLooper()
    {
        stop_ = true;
        ZBACKUP_LOG_INFO("BackupLooper stopped");
    }

    // 热点文件监控主循环
    void BackupLooper::hot_monitor() const
    {
        auto& container = core::ServiceContainer::get_instance();
        auto config = container.resolve<interfaces::IConfigManager>();
        
        if (!config) {
            ZBACKUP_LOG_FATAL("ConfigManager not available for BackupLooper");
            return;
        }
        
        std::string back_dir = config->get_string("back_dir", "./backup/");
        int hot_time = config->get_int("hot_time", 300);

        ZBACKUP_LOG_INFO("Hot file monitor started, checking every 1s for files idle > {}s", hot_time);

        // 持续监控循环
        while (!stop_)
        {
            // 1. 遍历备份目录，获取所有文件名
            util::FileUtil fu(back_dir);
            std::vector<std::string> arry;
            fu.scan_directory(&arry);

            int hot_file_count = 0;
            // 2. 判断是否为热点文件
            for (auto &str: arry)
            {
                if (hot_judge(str, hot_time) == false)
                    continue;

                hot_file_count++;
                core::ThreadPool::get_instance()->submit_task([this, str]()
                {
                    this->deal_task(str);
                });
            }

            if (hot_file_count > 0)
            {
                ZBACKUP_LOG_INFO("Found {} hot files to compress", hot_file_count);
            }

            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    // 处理热点文件的压缩任务
    void BackupLooper::deal_task(const std::string &str) const
    {
        auto& container = core::ServiceContainer::get_instance();
        auto data_manager = container.resolve<interfaces::IDataManager>();
        
        if (!data_manager) {
            ZBACKUP_LOG_ERROR("DataManager not available for task processing");
            return;
        }

        // 3. 获取文件信息
        info::BackupInfo bi;
        if (data_manager->get_one_by_real_path(str, &bi) == false)
        {
            ZBACKUP_LOG_WARN("File [{}] exists but no backup info found, creating new entry", str);
            bi.new_backup_info(str);
        }

        // 4. 对热点文件进行压缩
        if (!comp_->compress(str, bi.pack_path_))
        {
            ZBACKUP_LOG_ERROR("Failed to compress hot file: {}", str);
            return;
        }

        util::FileUtil tmp(str);
        // 5. 删除源文件
        if (!tmp.remove_file())
        {
            ZBACKUP_LOG_ERROR("Failed to remove source file after compression: {}", str);
            // 如果删除源文件失败，也删除压缩文件
            util::FileUtil pack_tmp(bi.pack_path_);
            pack_tmp.remove_file();
            return;
        }

        bi.pack_flag_ = true;
        if (data_manager->update(bi) == false)
        {
            ZBACKUP_LOG_ERROR("Failed to update backup info for: {}", str);
            return;
        }

        ZBACKUP_LOG_INFO("Hot file compressed successfully: {}", str);
    }

    // 判断文件是否为热点文件（长时间未访问）
    bool BackupLooper::hot_judge(const std::string &filename, const int hot_time)
    {
        util::FileUtil fu(filename);
        time_t last_atime = fu.get_last_atime();
        time_t cur_time = time(nullptr);
        return (cur_time - last_atime > hot_time);
    }
}
