#pragma once
#include "../compress/compress.h"
#include "../data/data_manage.h"
#include "../core/threadpool.h"
#include <atomic>
#include <memory>

namespace zbackup
{
    class BackupLooper
    {
    public:
        using ptr = std::shared_ptr<BackupLooper>;

        explicit BackupLooper(Compress::ptr comp);

        ~BackupLooper();

        void start() const;

    private:
        void hot_monitor() const;

        void deal_task(const std::string &str) const;

        static bool hot_judge(const std::string &filename, int hot_time);

    private:
        std::atomic<bool> stop_;
        Compress::ptr comp_;
    };
}
