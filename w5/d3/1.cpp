#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main()
{
    pid_t pid = fork();

    if (pid < 0) {
        // fork 失败
        perror("fork");
        return 1;
    } else if (pid == 0) {
        // ---------- 子进程 ----------
        // 准备参数数组。注意：第一个元素必须是程序名本身，最后一个元素必须是 nullptr
        char* argv[] = { (char*)"ls", (char*)"-l", nullptr };
        
        execvp("ls", argv);
        
        // 如果执行到了这里，说明 execvp 失败了（比如找不到 ls 命令）
        perror("execvp"); 
        _exit(127); // 用 _exit 而不是 exit，避免刷新父进程继承来的缓冲区
    } else {
        // ---------- 父进程 ----------
        int status;
        waitpid(pid, &status, 0); // 等待子进程结束并回收
        
        if (WIFEXITED(status)) {
            std::cout << "子进程正常退出，退出码: " << WEXITSTATUS(status) << std::endl;
        }
    }

    return 0;
}