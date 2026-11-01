#include<iostream>
#include<fcntl.h>
#include<unistd.h>


int main ()
{
    int fd1=::open("1.txt", O_RDONLY| O_CREAT);
    std::cout<<fd1<<"\n";
    int fd2= open("b.txt", O_WRONLY|O_CREAT|O_TRUNC, 0644);
    

}
