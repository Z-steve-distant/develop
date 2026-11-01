#include<functional>
#include<iostream>
#include<string>
#include<utility>
#include<vector>
class TaskRunner
{
public:
    void add(std::function<void()> task)
    {
        task_.push_back(std::move(task));
    }
    void runAll()
    {
        for(auto& task:task_)
        {
            task();
        }
        task_.clear();
    }
    std::size_t size()const{return task_.size();}
private:
    std::vector<std::function<void()>> task_;
};
int main()
{
    TaskRunner runner;
    int count=0;
    runner.add([count]()mutable{
       std::cout << "callback 1: count = " << count << "\n";
    });
    std::string msg="Hellow";
    runner.add([msg=std::move(msg)]{
         std::cout << "callback 2: " << msg << "\n";
    });
    int total=0;
    runner.add([&total]{
        total+=42;
         std::cout << "callback 3: total = " << total << "\n";
    });
      std::cout << "before run, size = " << runner.size() << "\n";
    runner.runAll();
    std::cout << "after run, size = " << runner.size() << "\n";
    std::cout << "total = " << total << "\n";

}