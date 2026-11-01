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
        _exit(3);

    }else
    {
        int status;
        waitpid(pid,&status,0);
        std::cout<<"父进程"<<std::endl;
  
        std::cout<<WIFEXITED(status)<<std::endl;
        std::cout<<WEXITSTATUS(status)<<std::endl;

    }

}