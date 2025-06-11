#pragma once
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <unordered_map>
#include <functional>
#include <queue>
#include <future>
#include <chrono>
#include "../log/logger.h"

namespace zbackup
{
    // 线程池配置常量
    static const size_t DEFAULT_THREAD_NUM = std::thread::hardware_concurrency(); // 默认线程数量（CPU核心数）
    static constexpr size_t MAX_THREAD_LIMIT = 16; // 最大线程数量限制
    static constexpr size_t MAX_TASK_DEFAULT_NUM = 10000; // 默认任务队列最大长度
    static constexpr std::chrono::seconds MAX_IDLE_TIME = std::chrono::seconds(10); // 线程最大空闲时间
    static constexpr std::chrono::seconds SUBMIT_TIME = std::chrono::seconds(1); // 任务提交最大等待时间

    // 类型别名定义
    using Task = std::function<void()>; // 任务类型
    using ThreadId = std::thread::id; // 线程ID类型

    // 线程池工作模式枚举
    enum class PoolMode
    {
        MODE_FIXED, // 固定线程数模式
        MODE_CACHED // 动态线程数模式
    };

    // 普通线程类
    class NormalThread
    {
    public:
        NormalThread() = default;

        virtual ~NormalThread() = default;

        NormalThread(NormalThread &) = delete; // 禁止拷贝构造
        NormalThread &operator=(NormalThread &) = delete; // 禁止拷贝赋值
        NormalThread(NormalThread &&) = default; // 支持移动构造
        NormalThread &operator=(NormalThread &&) = default; // 支持移动赋值

        // 启动线程并执行任务
        void start(const Task &task)
        {
            std::unique_ptr<std::thread> p_thread(new std::thread(task));
            thread_ = std::move(p_thread);
        }

        // 获取线程ID
        [[nodiscard]] ThreadId get_id() const { return thread_->get_id(); }

        // 等待线程结束
        void join() const
        {
            if (thread_->joinable())
            {
                thread_->join();
            }
        }

    protected:
        std::unique_ptr<std::thread> thread_; // 线程对象
    };

    // 执行任务线程类
    class ExcuteThread : public NormalThread
    {
    public:
        ExcuteThread() = default;

        ~ExcuteThread() override = default;
    };

    // 监视线程类
    class MonitorThread : public NormalThread
    {
    public:
        MonitorThread() = default;

        ~MonitorThread() override = default;
    };

    // 线程池配置类
    class PoolInfo
    {
    public:
        PoolInfo()
            : quit_(false), cur_thread_nums_(0),
              idle_thread_nums_(0), max_thread_nums_(0),
              task_max_size_(MAX_TASK_DEFAULT_NUM), pool_mode_(PoolMode::MODE_FIXED)
        {
        }

        virtual ~PoolInfo() = default;

        // 检查线程池是否处于运行状态
        [[nodiscard]] bool check_running_state() const { return !quit_; }

        // 获取最大线程数
        [[nodiscard]] size_t get_max_count() const { return max_thread_nums_.load(); }

        // 获取活动线程数
        [[nodiscard]] size_t get_active_count() const { return cur_thread_nums_.load(); }

        // 获取任务队列的最大任务数
        [[nodiscard]] size_t get_task_max_count() const { return task_max_size_.load(); }

        // 获取空闲线程数
        [[nodiscard]] size_t get_idle_count() const { return idle_thread_nums_.load(); }

        // 获取线程池模式
        [[nodiscard]] PoolMode get_pool_mode() const { return pool_mode_; }

        // 设置任务队列最大任务数
        void set_task_max_count(size_t new_size) { task_max_size_ = new_size; }

    protected:
        std::atomic<bool> quit_; // 是否退出标志
        std::atomic<size_t> cur_thread_nums_; // 当前线程数
        std::atomic<size_t> idle_thread_nums_; // 空闲线程数
        std::atomic<size_t> max_thread_nums_; // 最大线程数
        std::atomic<size_t> task_max_size_; // 任务队列最大长度
        PoolMode pool_mode_; // 线程池模式
    };

    class ThreadPool final : public PoolInfo
    {
    public:
        // 获取线程池单例
        static ThreadPool *get_instance()
        {
            // once_flag与call_once保证单例只执行一次
            std::call_once(flag_, []()
            {
                instance_.reset(new ThreadPool());
            });
            return instance_.get();
        }

        // 启动线程池
        void start(size_t max_thread_nums = DEFAULT_THREAD_NUM, PoolMode mode = PoolMode::MODE_FIXED)
        {
            max_thread_nums_ = max_thread_nums;
            pool_mode_ = mode;

            // 启动初始线程
            for (size_t i = 0; i < max_thread_nums; i++)
            {
                launch_ex_thread();
            }

            // 启动监视线程
            if (pool_mode_ == PoolMode::MODE_CACHED)
            {
                launch_mo_thread();
            }
        }

        // 提交任务到线程池
        template<typename Func, typename... Ts>
        auto submit_task(Func &&func, Ts &&... params)
            -> std::future<decltype(func(params...))>
        {
            // 1. 封装任务
            auto execute = std::bind(std::forward<Func>(func), std::forward<Ts>(params)...); // 创建绑定函数
            using ReturnType = decltype(func(params...));
            using PackagedTask = std::packaged_task<ReturnType()>;
            auto task = std::make_shared<PackagedTask>(execute); // 创建任务包装器
            auto result = task->get_future(); // 获取异步任务的结果

            // 2. 加入任务队列，并且判断是否需要增加线程
            {
                std::unique_lock<std::mutex> lock(mtx_); // 锁住线程池

                // 等待超过1s，任务提交失败
                if (!not_full_.wait_for(lock, SUBMIT_TIME, [this]()
                {
                    return tasks_queue_.size() < task_max_size_;
                }))
                {
                    ZBACKUP_LOG_WARN("Task queue is full, submit task failed");
                    auto fail_task = std::make_shared<PackagedTask>([]()
                    {
                        return ReturnType();
                    });
                    (*fail_task)();
                    return fail_task->get_future();
                }

                tasks_queue_.emplace([task]()
                {
                    (*task)();
                }); // 将任务加入队列
            }

            if (idle_thread_nums_ > 0)
            {
                not_empty_.notify_one(); // 通知空闲线程处理任务
            }
            else if (cur_thread_nums_ < max_thread_nums_ && pool_mode_ == PoolMode::MODE_CACHED)
            {
                launch_ex_thread(); // 启动新线程处理任务
            }
            return result; // 返回任务的future对象
        }

        // 清理单例资源
        ~ThreadPool() override
        {
            // 1. 设置标记位，通知所有线程退出
            quit_ = true;
            not_empty_.notify_all(); // 通知所有线程退出
            not_full_.notify_all(); // 通知所有线程退出
            time_out_.notify_all(); // 通知监视线程退出

            // 2. 回收监视线程
            if (pool_mode_ == PoolMode::MODE_CACHED && monitor_thread_)
                monitor_thread_->join();

            // 3. 回收执行任务所有线程
            for (const auto &excute_thread: excute_threads_)
            {
                // 回收每个线程
                excute_thread.second->join();
            }
        }

    private:
        // 构造函数，初始化线程池
        ThreadPool() = default;

        // 执行任务
        void excute_task()
        {
            while (true)
            {
                Task task;

                // 1. 从任务队列提取任务，判断任务是否需要放入超时回收队列
                {
                    std::unique_lock<std::mutex> lock(mtx_);
                    ThreadId thread_id = std::this_thread::get_id();

                    if (pool_mode_ == PoolMode::MODE_CACHED)
                    {
                        // 等待任务超时且任务队列为空
                        bool is_time_out = not_empty_.wait_for(lock, MAX_IDLE_TIME, [this]
                        {
                            return quit_ || !tasks_queue_.empty();
                        });

                        // 线程超时
                        if (!is_time_out)
                        {
                            // 不能低于默认大小
                            if (cur_thread_nums_ <= DEFAULT_THREAD_NUM)
                            {
                                not_empty_.wait(lock, [this]
                                {
                                    return quit_ || !tasks_queue_.empty();
                                });
                            }
                            else
                            {
                                --idle_thread_nums_;
                                --cur_thread_nums_;
                                time_out_queue_.emplace(thread_id); // 超时线程加入回收队列
                                time_out_.notify_all();
                                return;
                            }
                        }
                    }
                    else
                    {
                        not_empty_.wait(lock, [this]
                        {
                            return quit_ || !tasks_queue_.empty();
                        });
                    }

                    if (quit_ && tasks_queue_.empty()) // 退出条件
                    {
                        return;
                    }

                    task = std::move(tasks_queue_.front()); // 获取任务
                    tasks_queue_.pop();
                    --idle_thread_nums_; // 空闲线程数减少
                }

                // 2. 执行任务
                not_empty_.notify_one(); // 还有任务可执行继续通知
                not_full_.notify_one(); // 通知可继续生产任务
                task();

                // 3. 修改线程状态
                ++idle_thread_nums_; // 空闲线程数增加
            }
        }

        // 启动一个执行任务线程
        void launch_ex_thread()
        {
            std::unique_ptr<ExcuteThread> excu(new ExcuteThread());
            excu->start([this] { excute_task(); });
            ThreadId id = excu->get_id();
            excute_threads_[id] = std::move(excu);
            ++cur_thread_nums_; // 当前线程数增加
            ++idle_thread_nums_; // 空闲线程数增加
        }

        // 启动一个监视线程
        void launch_mo_thread()
        {
            auto moni = std::make_unique<MonitorThread>();
            moni->start([this] { monitor_idle_threads(); });
            monitor_thread_ = std::move(moni);
        }

        // 监视空闲线程并回收超时线程，打印日志
        void monitor_idle_threads()
        {
            while (!quit_)
            {
                {
                    std::unique_lock<std::mutex> lock(mtx_);
                    time_out_.wait(lock, [this]()
                    {
                        return !time_out_queue_.empty() || quit_;
                    });

                    // 回收线程
                    while (!time_out_queue_.empty())
                    {
                        ThreadId thread_id = time_out_queue_.front();
                        time_out_queue_.pop();
                        excute_threads_[thread_id]->join();
                    }
                }
                // 打印线程池状态
                ZBACKUP_LOG_DEBUG("ThreadPool status - Max: {}, Current: {}, Idle: {}",
                                  max_thread_nums_.load(), cur_thread_nums_.load(), idle_thread_nums_.load());
            }
        }

    private:
        std::mutex mtx_; // 互斥锁
        std::queue<Task> tasks_queue_; // 任务队列
        std::condition_variable not_empty_; // 任务可用条件变量
        std::condition_variable not_full_; // 任务可生产条件变量
        std::condition_variable time_out_; // 超时回收条件变量
        std::unordered_map<ThreadId, std::unique_ptr<ExcuteThread> > excute_threads_; // 存储执行线程信息
        std::unique_ptr<MonitorThread> monitor_thread_; // 监视空闲线程
        std::queue<ThreadId> time_out_queue_; // 超时线程队列

        inline static std::unique_ptr<ThreadPool> instance_;
        inline static std::once_flag flag_;
    };
};
