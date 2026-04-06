#include "CurrentThread.h"

#include <sys/syscall.h>
#include <unistd.h>

namespace CurrentThread {
thread_local int t_cacheTid = 0;

void cacheTid() {
    if (t_cacheTid == 0) {
        // 使用syscall获取内核级线程ID
        t_cacheTid = static_cast<pid_t>(::syscall(SYS_gettid));
    }
}
}  // namespace CurrentThread
