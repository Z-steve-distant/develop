#include<iostream>
#include<cstdlib>
#include<unistd.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<cerrno>
#include<cstring>

int main()
{
    for(int i=1;i<=5;i++)
    {
        std::cout<<i<<" ";
    }
    std::cout<<std::endl;
    pid_t pid=fork();
    if(pid==0)
    {
        for(int i=10;i<=100;i++)
        {
            std::cout<<"-"<<i<<" ";
        }
    std::cout<<std::endl;
//WNOHANG  wnohang
    }else
    {
        int status;
    int r=   waitpid(pid,&status,WNOHANG);
    std::cout<<r<<std::endl;
    sleep(1);
        std::cout<<"父进程"<<std::endl;
        sleep(10);
int st; pid_t w;
while ((w = waitpid(-1, &st, WNOHANG)) > 0)
    printf("回收到 pid=%d status=%d\n", w, st);

    }

}