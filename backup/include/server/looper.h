/**
 * @file looper.h
 * @brief 后台循环器头文件，负责监控热点文件并进行压缩处理
 */

#pragma once
#include "interfaces/compress_interface.h"
#include <memory>
#include <atomic>
#include <thread>

namespace zbackup
{
    /**
     * @class BackupLooper
     * @brief 后台循环器，监控热点文件并自动压缩
     */
    class BackupLooper
    {
    public:
        using ptr = std::shared_ptr<BackupLooper>;

        explicit BackupLooper(interfaces::ICompress::ptr comp);
        ~BackupLooper();

        // 启动热点监控
        void start() const;

    private:
        // 热点监控主循环
        void hot_monitor() const;
        
        // 处理热点文件压缩任务
        void deal_task(const std::string &str) const;
        
        // 判断文件是否为热点文件（长时间未访问）
        static bool hot_judge(const std::string &filename, int hot_time);
    
    private:
        std::atomic<bool> stop_;              // 停止标志
        interfaces::ICompress::ptr comp_;     // 压缩器接口
    };
}
