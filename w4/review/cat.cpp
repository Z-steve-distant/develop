#include<cstdio>
#include<cerrno>
#include<unistd.h>
#include<vector>
#include<fcntl.h>
#include<cstring>

namespace{
class FileDescriptor
{
private:
    int fd_=-1;
public:
    FileDescriptor()=default;
    explicit FileDescriptor(int fd):fd_(fd){}

    ~FileDescriptor(){
        ::close(fd_);
    }
    FileDescriptor(FileDescriptor&)=delete;

    FileDescriptor(FileDescriptor&& other){
        fd_=other.release();
    }

    FileDescriptor& operator=(FileDescriptor&)=delete;

    FileDescriptor& operator=(FileDescriptor&& other){
       if(this!=&other)
       {
        reset(other.release());
       }
       return *this;
        
    }


    int release()
    {
        int mid=fd_;
        fd_=-1;
        return mid;
    }
    void reset(int fd=-1)
    {
        if(fd_>=0&&fd_!=fd)
        {
            ::close(fd_);
        }
        fd_=fd;
    }
    [[nodiscard]] int get(){return fd_;}
    [[nodiscard]] bool valid(){return fd_>=0;}
};
ssize_t ReadRetry(int fd,void* buf,std::size_t count){
    ssize_t n=0;
    do{

        n=::read(fd,buf,count);

    }while(n==-1&&errno==EINTR);

    return n;
}
bool WriteAll(int fd,void* buf,std::size_t count)
{
    std::size_t written=0;
    while(written<count)
    {
        const ssize_t n=::write(fd,static_cast<const char*>(buf)+written,count-written);
        if(n==-1)
        {
            if(errno==EINTR) continue;
            else return false;
        }
        if (n==0)
        {
            return false;

        }
        
        written+=static_cast<std::size_t>(n);

    }
    return true;
}
bool catStream(int fd,const char* displayName)
{
    std::vector<char> buf(64*1024);
    for(;;)
    {
    ssize_t n=ReadRetry(fd,buf.data(),buf.size());
    if(n==0) return true;
    if(n<0){
        std::fprintf(stderr,"mycat:%s---%s\n",displayName,strerror(errno));
        return false;
    }

    if(!WriteAll(STDOUT_FILENO,buf.data(),n))
    {
        std::fprintf(stderr,"my_cat:%s\n",strerror(errno));
        return false;
    }
    }
}
bool catOne(const char* path)
{
    if (std::strcmp(path, "-") == 0) return catStream(STDIN_FILENO, "-");

    FileDescriptor fd(::open(path, O_RDONLY));
    if (!fd.valid())
    {
        std::fprintf(stderr, "my_cat:%s: %s\n", path, std::strerror(errno));
        return false;
    }
    return catStream(fd.get(), path);
}

}
int main(int argc, char** argv)
{
    if (argc == 1) return catStream(STDIN_FILENO, "-") ? 0 : 1;

    bool allok = true;                       
    for (int i = 1; i < argc; ++i)
        allok &= catOne(argv[i]);            
    return allok ? 0 : 1;
}

