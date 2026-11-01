#include<cerrno>
#include<cstdio>
#include<cstring>
#include<fcntl.h>
#include<unistd.h>
#include<vector>
namespace{

class FileDescriptor{
public:
    FileDescriptor()noexcept=default;
    explicit FileDescriptor(int fd) noexcept: fd_(fd){}
    ~FileDescriptor(){reset();}
    FileDescriptor(const FileDescriptor&)=delete;
    FileDescriptor& operator =(const FileDescriptor&)=delete;

    FileDescriptor(FileDescriptor&& other)noexcept:fd_(other.release()){}
    FileDescriptor& operator=(FileDescriptor&& other)noexcept{
        if(this!=&other)
        {
            reset();
            fd_=other.release();
        }
        return *this;
    }
    [[nodiscard]] int get()const noexcept{return fd_;}
    [[nodiscard]] bool valid()const noexcept{return fd_>=0;}

    int release()noexcept{
        int tmp=fd_;
        fd_=-1;
        return tmp;
    }

    void reset(int newFd=-1)noexcept{
        if(fd_>=0)
        {
            ::close(fd_);
        }
        fd_=newFd;
    }
private:
    int fd_=-1;
};
ssize_t readRetry(int fd,void* buf,std::size_t count)
{
    ssize_t n;
    do{
        n=::read(fd,buf,count);

    }while(n==-1&&errno==EINTR);
    return n;
}
bool writeAll(int fd,const char* data,std::size_t count)
{
    std::size_t written=0;
    while(written<count)
    {
        const  ssize_t n=::write(fd,data+written,count-written);
        if(n==-1)
        {
            if(errno==EINTR)continue;
            return false;
        }
        if(n==0) return false;
        written+=static_cast<std::size_t>(n);
    }
    return true;
}
bool catStream(int fd,const char* displayName)
{
    std::vector<char> buf(64*1024);
    for(;;)
    {
        const ssize_t n=readRetry(fd,buf.data(),buf.size());
        if(n==0) return true;
        if(n<0)
        {
            std::fprintf(stderr,"my_cat:%s:%s\n",displayName,std::strerror(errno));
            return false;
        }
        if(!writeAll(STDOUT_FILENO,buf.data(),static_cast<std::size_t>(n)))
        {
                std::fprintf(stderr,"my_cat:%s\n",std::strerror(errno));
                return false;
        }


    }
}
bool catOne(const char* path,bool& ok)
{
    if(std::strcmp(path,"-")==0)
    {
        ok=catStream(STDIN_FILENO,"-");
        return true;
    }
    FileDescriptor fd(::open(path,O_RDONLY));
    if(!fd.valid())
    {
        std::fprintf(stderr,"my_cat:%s:%s\n",path,std::strerror(errno));
        ok=false;
        return true;
    }
    ok=catStream(fd.get(),path);
    return true;
}


}

int main(int argc,char** argv)
{
    if(argc==1)
    {
        return catStream(STDIN_FILENO,"-")?0:1;
    }
    bool allok=true;
    for(int i=1;i<argc;i++)
    {
        catOne(argv[i],allok);
    }
    return allok?0:1;
}
