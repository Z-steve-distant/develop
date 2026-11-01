// 周六 · TaskRunner：注册并执行 ≥3 个回调（W3 验收 A 档第 1 条）
#include "w3/task_runner.hpp"

#include <cstdio>
#include <memory>
#include <string>
#include <utility>

namespace {

struct Repository {
    std::string name = "repo";
    void save() const { std::printf("  [6] Repository::save（成员函数 → lambda，%s）\n", name.c_str()); }
};

}  // namespace

int main() {
    Repository repo;   // 注意：repo 比 runner 先声明，保证它活得更久
    w3::TaskRunner runner;
    int value = 0;

    // 1) 无捕获 lambda：等价于裸函数指针，最轻
    runner.add("plain", [] { std::printf("  [1] 无捕获 lambda\n"); });

    // 2) 值捕获：闭包自己持有一份副本，外部变量不受影响（安全默认）
    runner.add("by-value", [value]() mutable {
        std::printf("  [2] 值捕获的副本 value=%d（外部不变）\n", ++value);
    });

    // 3) 引用捕获：能改外部，但要求"回调执行时变量还活着"——异步场景的经典悬垂来源
    runner.add("by-ref", [&value] { std::printf("  [3] 引用捕获并自增，value=%d\n", ++value); });

    // 4) shared_ptr 捕获：共享所有权，可以安全地跨作用域/跨线程
    auto box = std::make_shared<int>(100);
    runner.add("shared_ptr", [box] {
        std::printf("  [4] shared_ptr 捕获 *box=%d，use_count=%ld\n", *box,
                    static_cast<long>(box.use_count()));
    });

    // 5) 移动捕获（C++14 init capture）：把所有权移进闭包
    //    std::function 要求目标可拷贝，unique_ptr 的 move-only 闭包放不进来；
    //    这里用可拷贝的 shared_ptr 演示 move 捕获，纯 move-only 闭包见 capture_demo.cpp
    auto owned = std::make_shared<int>(7);
    runner.add("shared-ptr-move", [p = std::move(owned)] { std::printf("  [5] 移动捕获 *p=%d\n", *p); });

    // 6) 成员函数调用 → lambda（std::bind 的现代替代）
    runner.add("member", [&repo] { repo.save(); });

    std::printf("注册了 %zu 个回调：\n", runner.size());
    std::size_t executed = runner.runAll();
    std::printf("执行了 %zu 个回调；被引用捕获改过的外部 value = %d\n", executed, value);

    return executed >= 3 ? 0 : 1;
}
