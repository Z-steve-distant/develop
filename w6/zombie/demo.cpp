#include <cstdio>
#include <sys/wait.h>
#include <unistd.h>

int main() {
    const pid_t pid = fork();
    if (pid < 0) { perror("fork"); return 1; }

if (pid == 0) {
    printf("[子] 父=%d\n", (int)getppid());
    fflush(stdout);
    sleep(5);                        // 父在这期间退出
    printf("[子] 现在父=%d（变了就是孤儿）\n", (int)getppid());
} else {
    return 0;                        // 父立刻走，别 wait
}
}