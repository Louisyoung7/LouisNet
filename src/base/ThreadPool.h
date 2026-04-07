#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <vector>

#include "noncopyable.h"

namespace base {
class ThreadPool : public noncopyable {
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex mutex_;
    std::condition_variable cv_;
    std::atomic_bool stop_{true};

   public:
    ThreadPool(size_t numThreads) : stop_(false) {
        for (size_t i = 0; i < numThreads; ++i) {
            workers_.emplace_back([this]() {
                while (true) {
                    std::function<void()> task;

                    {
                        std::unique_lock<std::mutex> lock(mutex_);
                        // 等待条件变量
                        cv_.wait(lock, [this]() { return this->stop_ || !this->tasks_.empty(); });
                        // 如果线程池停止，退出循环
                        if (this->stop_)
                            return;

                        // 获取任务
                        task = std::move(this->tasks_.front());
                        this->tasks_.pop();
                    }

                    // 释放锁后执行任务
                    task();
                }
            });
        }
    }

    ~ThreadPool() {
        stop_ = true;
        // 通知所有等待线程停止
        cv_.notify_all();

        for (auto& worker : workers_) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

    template <typename Func, typename... Args>
    auto submit(Func&& f, Args&&... args) -> std::future<std::invoke_result_t<Func, Args...>> {
        using ReturnType = std::invoke_result_t<Func, Args...>;

        // 包装任务
        // packaged_task不支持拷贝，而任务需要在多个地方流转（Lambda表达式、任务队列、执行线程等）
        // 共享指针保证任务可拷贝，同时正确管理生命周期
        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
            std::bind(std::forward<Func>(f), std::forward<Args>(args)...));

        // 获取future对象
        std::future<ReturnType> result = task->get_future();

        {
            std::lock_guard<std::mutex> lock(mutex_);

            // 禁止向已停止的线程池添加任务
            if (stop_) {
                throw std::runtime_error("Submit in stopped threadPool.");
            }

            // 将task共享指针捕获并包装成Lambda表达式
            tasks_.emplace([task]() { (*task)(); });
        }

        // 通知一个线程
        cv_.notify_one();

        return result;
    }
};
}  // namespace base
