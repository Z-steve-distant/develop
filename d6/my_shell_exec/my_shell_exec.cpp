#include<cerrno>
#include<cstdio>
#include<cstdlib>
#include<cstring>
#include<string>
#include<sys/types.h>
#include<sys/wait.h>
#include<unistd.h>
namespace{
[[noreturn]] void childExec(char* const argv[]){
    std::printf("[子]pid=%d ppid=%d 准备exec[%s] \n",static_cast<int>(getpid()),
    static_cast<int>(getppid()),argv[0]);
    std::fflush(stdout);
    execvp(argv[0],argv);

    std::fprintf(stderr,"[子]execvp(\"%s\n)失败：%s\n",argv[0],std::strerror(errno));
    _exit(127);

}
int waitChild(pid_t pid)
{
    int status=0;
    for(;;){
        const pid_t  r=waitpid(pid,&status,0);
        if(r==pid) break;
        if(r<0&&errno==EINTR)continue;
        std::perror("[父]waitpid");
        return -1;
    }
    std::printf("[父]回收子进程 pid=%d,原始status=0x%04x (%d)\n",static_cast<int>(pid),static_cast<unsigned>(status),status);

    if(WIFEXITED(status))
    {
        std::printf("[父] WIFEXITED=1 WEXITSTATUS=%d\n",WEXITSTATUS(status));
    }else if(WIFSIGNALED(status))
    {
        std::printf("[父] WIFSIGNALED=1  WTERMSIG=%d (%s)\n", WTERMSIG(status), strsignal(WTERMSIG(status)));
    }
    return status;

}
int runOne(char* const argv[])
{
    std::fflush(stdout);
    const pid_t pid=fork();
    if(pid<0)
    {
        std::perror("[父]fork");
        return -1;
    }
    if(pid==0)
    {
        childExec(argv);
    }
    std::printf("[父] pid=%d 创建了子进程 pid=%d\n",static_cast<int>(getpid()),static_cast<int>(pid));
    const int status=waitChild(pid);
    if(status<0) return -1;
    if(WIFEXITED(status)) return WEXITSTATUS(status);
    if(WIFSIGNALED(status)) return 128+WTERMSIG(status);
    return -1;

}
bool splitLine(const std::string& line, char** argv,int cap,std::string& store)
{
    store=line;
    int n=0;
    char* p=store.data();
    while(*p&&n<cap)
    {
        while(*p==' '||*p=='\t') ++p;
        if(!*p)break;
        argv[n++]=p;
        while(*p&&*p!=' '&&*p!='\t')++p;
        if(*p) *p++='\0';
    }
    argv[n]=nullptr;
    return n>0;
}


}

int main( int argc,char** argv)
{
    if(argc>1)
    {
        const int code=runOne(&argv[1]);
        std::printf("[父]本次命令退出码=%d\n",code);
        return code<0?1:code;
    }
    char buf[1024];
    for(;;){
        std::printf("mysh> ");
        std::fflush(stdout);
        if(std::fgets(buf, sizeof(buf),stdin)==nullptr) break;
       std::string line(buf);
       while(!line.empty()&&(line.back()=='\n'||line.back()=='\r'))line.pop_back();
       if(line.empty()) continue;
       if(line=="exit"||line=="break") break;

       char* args[64];
    std::string store;
        if(!splitLine(line,args,63,store))continue;
        const int code=runOne(args);
        std::printf("[父]上一条退出码=%d\n",code);




    }
    std::printf("[父]再见\n");
    return 0;
}