#include "../../include/server/looper.h"
#include "../../include/config/config.h"

namespace zbackup
{
    // 后台循环构造函数，启动热点文件监控
    BackupLooper::BackupLooper(Compress::ptr comp)
        : stop_(false), comp_(comp)
    {
        ZBACKUP_LOG_INFO("BackupLooper initialized");
    }

    void BackupLooper::start()
    {
        std::thread monitor_thread(&BackupLooper::hot_monitor, this);
        monitor_thread.detach(); // 分离线程，允许其在后台运行
        ZBACKUP_LOG_INFO("BackupLooper started monitoring for hot files");
    }
    // 析构函数，停止监控循环
    BackupLooper::~BackupLooper()
    {
        stop_ = true;
        ZBACKUP_LOG_INFO("BackupLooper stopped");
    }

    // 热点文件监控主循环
    void BackupLooper::hot_monitor()
    {
        // 读取配置
        Config &config = Config::get_instance();
        std::string back_dir = config.get_back_dir();
        int hot_time = config.get_hot_time();
        DataManager* data_manager = DataManager::get_instance();
        
        ZBACKUP_LOG_INFO("Hot file monitor started, checking every 1s for files idle > {}s", hot_time);
        
        // 持续监控循环
        while (!stop_)
        {
            // 1. 遍历备份目录，获取所有文件名
            FileUtil fu(back_dir);
            std::vector<std::string> arry;
            fu.scan_directory(&arry);

            int hot_file_count = 0;
            // 2. 判断是否为热点文件
            for (auto &str : arry)
            {
                if (hot_judge(str, hot_time) == false)
                    continue;
                    
                hot_file_count++;
                ThreadPool::get_instance()->submit_task([this, str]()
                                                      { this->deal_task(str); });
            }
            
            if (hot_file_count > 0) {
                ZBACKUP_LOG_INFO("Found {} hot files to compress", hot_file_count);
            }
            
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    // 处理热点文件的压缩任务
    void BackupLooper::deal_task(const std::string &str)
    {
        DataManager* data_manager = DataManager::get_instance();
        
        // 3. 获取文件信息
        BackupInfo bi;
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

        FileUtil tmp(str);
        // 5. 删除源文件
        if (!tmp.remove_file())
        {
            ZBACKUP_LOG_ERROR("Failed to remove source file after compression: {}", str);
            // 如果删除源文件失败，也删除压缩文件
            FileUtil pack_tmp(bi.pack_path_);
            pack_tmp.remove_file();
            return;
        }

        bi.pack_flag_ = true;
        if (data_manager->update(bi) == false)
        {
            ZBACKUP_LOG_ERROR("Failed to update backup info for: {}", str);
            return;
        }
        if (data_manager->persistence() == false)
        {
            ZBACKUP_LOG_ERROR("Failed to persist backup info for: {}", str);
            return;
        }
        
        ZBACKUP_LOG_INFO("Hot file compressed successfully: {}", str);
    }

    // 判断文件是否为热点文件（长时间未访问）
    bool BackupLooper::hot_judge(const std::string &filename, int hot_time)
    {
        FileUtil fu(filename);
        time_t last_atime = fu.get_last_atime();
        time_t cur_time = time(nullptr);
        return (cur_time - last_atime > hot_time);
    }
}
