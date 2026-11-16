#include<cerrno>
#include<cstdio>
#include<cstring>
#include<fcntl.h>
#include<limits.h>
#include<string>
#include<sys/wait.h>
#include<unistd.h>


int main(){
    int fd[2]={-1,-1};
    if(pipe(fd)!=0){
        std::perror("pipe");
        return;
    }
    std::printf("[父] pipe 建好：读端 fd[0]=%d，写端 fd[1]=%d\n", fd[0], fd[1]);
std::fflush(stdout);
const pid_t pid=fork();
if(pid<0){
    std::perror("fork");
    return 1;
}
if(pid==0){
    if(close(fd[1]!=0)){std::perror("close");_exit(1);}

char buf[64];
for(;;){
    const ssize_t n=read(fd[0],buf,sizeof(buf));
    if(n<0){
        if(errno==EINTR) continue;
        std::perror("read");
        _exit(1);
    }
    if(n==0) break;
    std::printf("[子] 读到 %zd 字节：%.*s", n, static_cast<int>(n), buf);
}
      std::printf("[子] 收到 EOF（read 返回 0），循环结束\n");
        std::fflush(stdout);
        _exit(0);

}
if(close(fd[0])!=0){std::perror("close"); return 1;}
int status=0;
    if(waitpid(pid,&status,0)<0){std::perror("waitpid");return 1;}
   std::printf("[父] 子进程已回收：WIFEXITED=%d WEXITSTATUS=%d\n",
                WIFEXITED(status), WEXITSTATUS(status));
                return 0;
}




