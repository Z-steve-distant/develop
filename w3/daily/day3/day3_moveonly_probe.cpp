// =============================================================================
//  day3_moveonly_probe.cpp   ——  【故意编译不过】std::function 要求 target 可拷贝
//
//  复现： g++ -std=c++20 -fsyntax-only day3_moveonly_probe.cpp
//  报错存档： day3_moveonly_error.txt
//
//  周三知识点：std::function 是**可拷贝**的类型擦除容器，它要求被存进去的可调用对象
//  也是可拷贝的。所以"捕获了 move-only 类型（unique_ptr 等）的 lambda"装不进去。
//  需要这种能力要用 C++23 的 std::move_only_function。
// =============================================================================
#include <functional>
#include <memory>

int main() {
    auto p = std::make_unique<int>(42);

    // 捕获 unique_ptr（move-only）→ std::function 装不下
    std::function<void()> f = [q = std::move(p)] { (void)q; };
    f();

    // 正确做法之一：共享所有权（shared_ptr 可拷贝）
    auto sp = std::make_shared<int>(42);
    std::function<void()> g = [sp] { (void)sp; };
    g();

    // 正确做法之二：把裸指针/引用存进去，所有权留在外面
    int value = 42;
    std::function<void()> h = [&value] { (void)value; };
    h();

    return 0;
}
