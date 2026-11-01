// 周一 · lambda 基础：闭包大小、mutable、与仿函数的等价性、转函数指针
#include <cstddef>
#include <cstdio>

namespace {

struct Functor {
    int k;
    int operator()(int x) const { return x + k; }
};

}  // namespace

int main() {
    int big[4] = {1, 2, 3, 4};
    int v = 7;

    auto l_none = [] {};
    auto l_val = [big] { (void)big; };
    auto l_ref = [&big] { (void)big; };
    auto l_v = [v] { return v; };
    auto l_r = [&v] { return v; };

    std::printf("闭包大小：无捕获=%zu  4×int值捕获=%zu  数组引用捕获=%zu  int值捕获=%zu  int引用捕获=%zu\n",
                sizeof(l_none), sizeof(l_val), sizeof(l_ref), sizeof(l_v), sizeof(l_r));
    std::printf("对照：    sizeof(int)=%zu  sizeof(int[4])=%zu  sizeof(int*)=%zu\n",
                sizeof(int), sizeof(int[4]), sizeof(int*));

    // mutable：改的是闭包自己的副本，外部变量不动
    int counter = 0;
    auto bump = [counter]() mutable { return ++counter; };
    int a1 = bump();
    int a2 = bump();
    int a3 = bump();
    std::printf("mutable 连调三次：%d %d %d ｜ 外部 counter 仍为 %d（改的是副本）\n", a1, a2, a3, counter);

    // lambda 等价于一个带 operator() 的仿函数
    int k = 5;
    auto lam = [k](int x) { return x + k; };
    Functor fun{k};
    std::printf("lambda(2)=%d  仿函数(2)=%d  %s\n", lam(2), fun(2),
                (lam(2) == fun(2)) ? "等价" : "不等价");

    // 只有无捕获 lambda 能隐式转成函数指针
    void (*fn_ptr)() = l_none;
    fn_ptr();
    std::printf("无捕获 lambda → 函数指针：%s\n", (fn_ptr != nullptr) ? "可以" : "不可以");

    return 0;
}
