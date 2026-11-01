#include <iostream>
#include <unistd.h>
#include <sys/wait.h>

int main()
{
    int x = 100;              // 栈上的变量，位于某个内存页
    std::cout << "fork 前: x=" << x
              << " 地址=" << &x << std::endl;

    pid_t pid = fork();

    if (pid == 0) {
        // ---- 子进程 ----
        std::cout << "[子] fork 后立刻读: x=" << x
                  << " 地址=" << &x << std::endl;

        x = 200;              // 写！触发 COW 缺页，复制该页
        std::cout << "[子] 写之后: x=" << x
                  << " 地址=" << &x << std::endl;
        _exit(0);
    } else {
        // ---- 父进程 ----
        waitpid(pid, nullptr, 0);   // 等子进程写完再读
        std::cout << "[父] 子进程结束后: x=" << x
                  << " 地址=" << &x << std::endl;
    }
    return 0;
}