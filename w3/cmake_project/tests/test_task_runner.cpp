#include "test_assert.hpp"
#include "w3/task_runner.hpp"

#include <memory>
#include <string>
#include <vector>

int main() {
    using w3::TaskRunner;

    // 1) 注册并执行 3 个回调（W3 验收 A 档第 1 条）
    {
        TaskRunner runner;
        CHECK(runner.empty());

        int total = 0;
        runner.add([&total] { total += 1; });
        runner.add("named", [&total] { total += 2; });
        std::size_t n = runner.add([&total] { total += 3; });

        CHECK(n == 3);
        CHECK(runner.size() == 3);
        CHECK(runner.runAll() == 3);
        CHECK(total == 6);
        CHECK(runner.names().size() == 3);
        CHECK(runner.names()[1] == "named");
    }

    // 2) 按注册顺序执行
    {
        TaskRunner runner;
        std::vector<int> order;
        runner.add([&order] { order.push_back(1); });
        runner.add([&order] { order.push_back(2); });
        runner.add([&order] { order.push_back(3); });
        runner.runAll();
        CHECK(order.size() == 3);
        CHECK(order[0] == 1);
        CHECK(order[1] == 2);
        CHECK(order[2] == 3);
    }

    // 3) shared_ptr 捕获：回调与外部共享所有权（跨作用域安全）
    {
        auto box = std::make_shared<int>(41);
        TaskRunner runner;
        runner.add([box] { *box += 1; });
        CHECK(box.use_count() == 2);
        runner.runAll();
        CHECK(*box == 42);
    }

    // 4) 移动捕获：把所有权移进闭包
    //    注意 std::function 要求目标可拷贝，unique_ptr 的 move-only 闭包放不进来；
    //    这里用可拷贝的 shared_ptr 演示 move 捕获，纯 move-only 闭包见 capture_demo.cpp
    {
        auto owned = std::make_shared<int>(7);
        TaskRunner runner;
        runner.add([p = std::move(owned)] { CHECK(*p == 7); });
        CHECK(owned == nullptr);
        CHECK(runner.runAll() == 1);
    }

    // 5) 空的 std::function 被跳过（不执行、不计数）
    {
        TaskRunner runner;
        runner.add(TaskRunner::Task{});
        CHECK(runner.size() == 1);
        CHECK(runner.runAll() == 0);
    }

    // 6) clear
    {
        TaskRunner runner;
        runner.add([] {});
        runner.clear();
        CHECK(runner.empty());
        CHECK(runner.runAll() == 0);
    }

    return w3test::summary("task_runner");
}
