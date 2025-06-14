#pragma once
#include "interfaces/compress_interface.h"
#include <memory>
#include <atomic>
#include <thread>

namespace zbackup
{
    class BackupLooper
    {
    public:
        using ptr = std::shared_ptr<BackupLooper>;

        explicit BackupLooper(interfaces::ICompress::ptr comp);
        ~BackupLooper();

        void start() const;

    private:
        void hot_monitor() const;
        void deal_task(const std::string &str) const;
        static bool hot_judge(const std::string &filename, int hot_time);
    
    private:
        std::atomic<bool> stop_;
        interfaces::ICompress::ptr comp_;
    };
}
