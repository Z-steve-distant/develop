// W3 验收 A 档第 2 条：同一逻辑用五种捕获方式各写一遍，注释写清生命周期风险
// 编译：g++ -std=c++20 -Wall -Wextra -O0 -g capture_demo.cpp -o capture_demo

#include <functional>
#include <iostream>
#include <memory>
#include <utility>

namespace {

// ① [=] 默认值捕获：捕获的是定义那一刻的快照
//    生命周期：闭包持有副本，变量先销毁也安全
//    风险：大对象会被整体拷贝；在成员函数里 [=] 实际捕获 this 指针
std::function<int()> byDefaultValue() {
    int x = 10;
    int y = 20;
    return [=] { return x + y; };  // 闭包里是 x、y 的副本
}

// ② [&] 默认引用捕获：闭包里存的是引用（底层是指针）
//    生命周期：回调执行时变量必须仍存活。返回带 [&] 的 lambda 即悬垂，调用是 UB
//    这里故意返回悬垂闭包，但 main 不调用它
std::function<int()> byDefaultRef() {
    int x = 10;
    int y = 20;
    return [&] { return x + y; };  // 危险：函数返回后 x / y 已销毁
}

// ③ [x] 显式值捕获：推荐写法，意图明确
//    生命周期：安全（副本）
//    风险：拷贝成本；要修改闭包内副本必须加 mutable
int explicitValue() {
    int x = 10;
    auto f = [x]() mutable { return ++x; };  // 改的是闭包里的副本
    return f() + f();                        // 11 + 12
}

// ④ [&x] 显式引用捕获：意图明确，生命周期仍要自己保证
//    风险：x 必须先于 lambda 销毁；异步回调场景最容易踩
int explicitRef() {
    int x = 10;
    auto f = [&x] { return ++x; };
    int r = f();  // x 变成 11
    return r + x; // 11 + 11
}

// ⑤ [y = std::move(z)] 初始化捕获（C++14）：把 move-only 对象搬进闭包
//    生命周期：所有权转移给闭包，z 变为空；闭包可以安全地活过 z
//    注意：捕获了 unique_ptr 的闭包不可拷贝，因此不能塞进 std::function（要求目标可拷贝）
//    用途：unique_ptr / 文件句柄等不可拷贝资源进 move-only 回调的标准写法
auto moveCapture() {
    auto z = std::make_unique<int>(42);
    return [y = std::move(z)] { return *y; };  // unique_ptr 只能移动捕获
}

// 补充：成员函数里的 [=] 等价于捕获 this 指针
// C++20 起隐式捕获 this 已弃用，写 [this] 才是无警告的显式写法
struct Widget {
    int v = 7;
    std::function<int()> callback() { return [this] { return v; }; }  // 捕获的是 this
};

}  // namespace

int main() {
    std::cout << "① [=]        " << byDefaultValue()() << "   副本快照，安全\n";

    auto dangling = byDefaultRef();
    (void)dangling;  // 不调用：返回后 x / y 已销毁，调用即 use-after-return
    std::cout << "② [&]        返回悬垂闭包（此处不调用）\n"
                 "             回调一律值捕获或 shared_ptr 捕获\n";

    std::cout << "③ [x]        " << explicitValue() << "   mutable 改的是闭包内副本\n";
    std::cout << "④ [&x]       " << explicitRef() << "   引用捕获修改外部变量\n";
    std::cout << "⑤ [y=move(z)] " << moveCapture()() << "   unique_ptr 移动进闭包\n";

    auto w = std::make_unique<Widget>();
    auto cb = w->callback();  // 闭包里只有 this
    w.reset();                // 对象销毁，cb 悬垂
    (void)cb;
    std::cout << "⑥ [this]     对象销毁后闭包悬垂（此处不调用）\n";

    return 0;
}
