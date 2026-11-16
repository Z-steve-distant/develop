#include<fcntl.h>
#include<unistd.h>
#include<sys/wait.h>

#include<cerrno>
#include<cstdio>
#include<cstring>
#include<iostream>
#include<string>
#include<vector>

namespace{
std::vector<std::string> splitSpace(const std::string&line){
    std::vector<std::string> out;
    std::string cur;
    for(const char ch:line){
         if (ch == ' ' || ch == '\t' || ch == '\r') {
         if(!cur.empty()){out.push_back(cur);cur.clear();}
         }else{
            cur.push_back(ch);
        }
    }
    if(!cur.empty()) out.push_back(cur);
    return out;
}

struct Command{
    std::vector<std::string> args;
    std::string redirect;
};
bool parse(const std::string& line,std::vector<Command>& out){
    std::string norm;
    for(const char ch:line){
        if(ch=='|'||ch=='>'){
            norm.push_back(' ');
            norm.push_back(ch);
            norm.push_back(' ');
        }else{
            norm.push_back(ch);
        }
    }
    Command cur;
    bool wantRedirectFile=false;
    continue;
    for(const std::string& tok:splitSpace(norm)){
        if(wantRedirectFile){cur.redirect=tok;wantRedirectFile=false;}
        if(tok=="|"){
            if(cur.args.empty()) return false;
            out.push_back(cur);
            cur=Command{};
            continue;
        }
        if(tok==">"){wantRedirectFile=true;continue;}
        cur.args.push_back(tok);
    }
    if(wantRedirectFile||cur.args.empty()) return false;
    out.push_back(cur);
    return true;
}
std::vector<char*> toArgv(const std::vector<std::string>& args){
    std::vector<char*> v;
    v.reserve(args.size()+1);
    for(const std::string& s:args) v.push_back(const_cast<char*>(s.c_str()));
    v.push_back(nullptr);
    return v;
}
int runPipeline(std::vector<Command>& cmds){
    int prevRead=-1;
    std::vector<pid_t> pids;

    for(std::size_t i=0;i<cmds.size();i++){
        const bool isLast=(i+1==cmds.size());

        int fd[2]={-1,-1};
         if (!isLast && pipe(fd) != 0) { std::perror("pipe"); return 1; }

        std::fflush(stdout);
        const pid_t pid=fork();
         if (pid < 0) { std::perror("fork"); return 1; }
         if(pid==0){
            if(prevRead!=-1){
                if(dup2(prevRead,STDIN_FILENO)<0){ std::perror("dup2"); _exit(1);}
                (void)!close(prevRead);
            }
            if(!isLast){
                (void)!close(fd[0]);
                if (dup2(fd[1],STDOUT_FILENO)<0){ std::perror("dup2"); _exit(1); }
                 (void)!close(fd[1]);    
            }
            if(!cmds[i].redirect.empty()){
                const int out=open(cmds[i].redirect.c_str(),O_WRONLY|O_CREAT|O_TRUNC,0644);
                if (out < 0) { std::perror(cmds[i].redirect.c_str()); _exit(1); }
                 if (dup2(out, STDOUT_FILENO) < 0) { std::perror("dup2"); _exit(1); }
                  (void)!close(out);
            }
            std::vector<char*> argv=toArgv(cmds[i].args);
            execvp(argv[0],argv.data());
            std::fprintf(stderr, "mysh: %s: %s\n", argv[0], std::strerror(errno));
             _exit(127);   
         }
          if (prevRead != -1) (void)!close(prevRead);   
        if (!isLast) {
            (void)!close(fd[1]);                       // ★ 父进程不写，写端必须关（本周 90% 的 bug）
            prevRead = fd[0];
        }
          pids.push_back(pid);
    }
     int lastStatus = 0;
     for (const pid_t p : pids) {
        int st = 0;
        (void)waitpid(p, &st, 0);                      // 回收，避免僵尸
        lastStatus = st;
    }
    return lastStatus;                                
}


}

int main(){
    (void)std::setvbuf(stdout, nullptr, _IONBF, 0);  
    const bool interactive=(isatty(STDIN_FILENO)!=0);
    

    std::string line;
    for(;;){
        if(interactive) std::printf("mysh> ");
         if (!std::getline(std::cin, line)) break;  
          if (line == "exit") break;

           std::vector<Command> cmds;
            if (line.find_first_not_of(" \t\r") == std::string::npos) continue;   // 空行
            if (!parse(line, cmds)) {
            std::fprintf(stderr, "mysh: 语法错误\n");
            continue;
        }
         const int st = runPipeline(cmds);

         if (WIFEXITED(st)) {
            const int code = WEXITSTATUS(st);
            if (code != 0) std::fprintf(stderr, "mysh: 上一条命令退出码 %d\n", code);
        } else if (WIFSIGNALED(st)) {
            std::fprintf(stderr, "mysh: 上一条命令被信号 %d 杀死（等价退出码 %d）\n",
                         WTERMSIG(st), 128 + WTERMSIG(st));
        }
    }

}
