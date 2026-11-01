#include<iostream>
#include <unistd.h>
#include<sys/wait.h>
int main()
{
pid_t pid = fork();
if (pid < 0) { perror("fork"); return 1; }
if (pid == 0) {
    printf("[子] getpid=%d getppid=%d\n", getpid(), getppid());
    /* ...原有循环... */
    _exit(0);
}
printf("[父] getpid=%d 子=%d\n", getpid(), pid);
waitpid(pid, nullptr, 0);
}