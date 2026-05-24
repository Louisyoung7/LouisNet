#pragma once

#include <sys/types.h>
#include <unistd.h>

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "base/noncopyable.h"
#include "timer/Timer.h"
#include "timer/TimerId.h"
#include "timer/TimerScheduler.h"

namespace net::reactor {
class Channel;
class Poller;

// EventLoop类
// 1.管理EventLoop内部的Poller实例
// 2.管理活跃的Channel列表
// 3.提供事件循环的启动和退出接口
// 4.提供Channel的添加和移除接口

class EventLoop : public base::noncopyable {
   public:
    using Functor = std::function<void()>;
    using ChannelList = std::vector<Channel*>;

    EventLoop();
    ~EventLoop();

    // 运行事件循环
    void loop();

    // 退出事件循环
    void quit();

    // 更新Channel
    void updateChannel(Channel* channel);

    // 移除Channel
    void removeChannel(Channel* channel);

    // 确保回调在loop线程执行
    void runInLoop(Functor cb);

    // 将回调放入任务队列，并唤醒loop线程
    void queueInLoop(Functor cb);

    // 判断是否在loop线程
    bool isInLoopThread();

    // 定时器相关接口
    // 在指定时间执行回调
    timer::TimerId runAt(timer::Timestamp time, timer::TimerCallback cb);
    // 在指定延迟后执行回调
    timer::TimerId runAfter(timer::Duration delay, timer::TimerCallback cb);
    // 每隔指定时间间隔执行回调
    timer::TimerId runEvery(timer::Duration interval, timer::TimerCallback cb);
    // 取消指定定时器
    void cancel(timer::TimerId timerId);

    // 唤醒loop线程
    void wakeup();

   private:
    std::unique_ptr<Poller> poller_;                         // 指向EventLoop内部的Poller实例
    ChannelList activeChannels_;                             // 活跃Channel列表
    std::atomic_bool looping_{false};                        // 是否正在循环
    std::atomic_bool quit_{false};                           // 是否停止事件循环
    std::vector<Functor> tasks_;                             // 任务列表
    std::mutex mutex_;                                       // 互斥锁，保证任务列表线程安全
    std::thread::id tid_;                                    // 记录EventLoop所属线程ID
    std::atomic_bool callingPendingFunctors_{false};         // 是否正在处理任务列表
    int eventfd_;                                            // 事件通知描述符
    std::unique_ptr<Channel> eventChannel_;                  // 事件通知Channel
    std::unique_ptr<timer::TimerScheduler> timerScheduler_;  // 定时器调度器

    // 调用Poller的poll，填充活跃的Channel列表
    void poll(int timeoutMs, ChannelList& activeChannels);

    // 执行待处理任务
    void doPendingFunctors();

    //
    void handleRead();
};
}  // namespace net::reactor