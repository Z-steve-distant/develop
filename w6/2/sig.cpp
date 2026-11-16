#include<unistd.h>
#include<sys/wait.h>

#include<cerrno>
#include<csignal>
#include<cstdio>
#include<cstring>

int main(int argc,char** argv){
  const bool ignoreSigpipe = (argc > 1 && std::strcmp(argv[1], "ignore") == 0);

    int fd[2]={-1,-1};
    if(pipe(fd)!=0){std::perror("pipe");return 1;}

    const pid_t pid=fork();
    if(pid<0){std::perror("fork");return 1;}

    if(pid==0){
        if(close(fd[1])!=0){std::perror("close");_exit(1);}
        char buf[128];
        const ssize_t n=read(fd[0],buf,sizeof(buf));
      std::printf("[读端] 读到 %zd 字节后立刻退出（它的 fd[0] 随进程退出被内核关闭）\n", n);
      std::fflush(stdout);
      _exit(0);
    }
    if(close(fd[0])!=0){std::perror("close");return 1;}

    if (ignoreSigpipe) {
        if (std::signal(SIGPIPE, SIG_IGN) == SIG_ERR) { std::perror("signal"); return 1; }
        std::printf("[写端] 已 signal(SIGPIPE, SIG_IGN)\n");
    } else {
        std::printf("[写端] 未处理 SIGPIPE（保持默认动作 = 杀进程）\n");
    }
    const char first[] = "first line\n";
    const ssize_t w1 = write(fd[1], first, sizeof(first) - 1);
    std::printf("[写端] 第一次 write 返回 %zd（读端还在，正常）\n", w1);
    std::fflush(stdout);

    int status = 0;
    (void)waitpid(pid, &status, 0);
    std::printf("[写端] 读端已退出。现在再写一次……\n");
    std::fflush(stdout);
     const char second[] = "second line\n";
    const ssize_t w2 = write(fd[1], second, sizeof(second) - 1);
    if (w2 < 0) {
        // 只有忽略/捕获了 SIGPIPE 才走得到这里
        std::printf("[写端] write 返回 -1，errno=%d (%s)\n", errno, std::strerror(errno));
        std::printf("[写端] 我还活着 —— 这就是网络编程必须忽略 SIGPIPE 的原因\n");
    } else {
        std::printf("[写端] write 返回 %zd（说明还有别的读者没关）\n", w2);
    }

    (void)close(fd[1]);
    return 0;



}