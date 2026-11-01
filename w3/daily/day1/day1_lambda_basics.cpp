// =============================================================================
//  W3 · 第一天（周一）· lambda 语法与"本质是匿名类"——全部断言都做成可编译验证
//
//  验收页周一的当日自检：能写出"编译器视角"的等价仿函数类。
//  本文件把这句话拆成 8 组可运行的断言，每一组都对应一个必须掌握的点。
//
//  编译： g++ -std=c++20 -Wall -Wextra -O0 -g day1_lambda_basics.cpp -o day1.exe
// =============================================================================
#include <cstddef>
#include <iostream>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>

static int g_pass = 0;
static int g_fail = 0;

#define CHECK(expr)                                                          \
    do {                                                                     \
        if (expr) { ++g_pass; }                                              \
        else { ++g_fail; std::cout << "  [FAIL] " << __LINE__ << ": " #expr "\n"; } \
    } while (0)

#define SHOW(label, value) \
    std::cout << "  " << label << " = " << (value) << '\n'

static void section(const char* t) { std::cout << "\n== " << t << " ==\n"; }

// ---------------------------------------------------------------------------
// 【1】完整语法骨架：每一部分都在哪
//      [捕获列表] <模板参数> (参数列表) 修饰符 noexcept -> 返回类型 { 函数体 }
//                                      ↑ mutable 在这里（参数列表之后、其他限定符之前）
// ---------------------------------------------------------------------------
static void t1_syntax_anatomy() {
    section("【1】语法骨架与各部分位置");
    int a = 1, b = 2;

    //            捕获列表   参数       mutable 返回类型  函数体
    //               ↓        ↓            ↓      ↓        ↓
    auto f = [a, &b](int x) /*mutable*/ -> long { b += x; return static_cast<long>(a) + b; };
    //                                 ↑ mutable 若写，必须紧跟在参数列表之后

    long r = f(10);
    SHOW("f(10) 返回", r);
    CHECK(r == 13);          // a=1 是值捕获的副本 → 1 + (2+10) = 13
    CHECK(b == 12);          // b 是引用捕获 → 外部变量被改
    SHOW("外部 b", b);

    // 立即调用（IIFE）：常用于 const 初始化的复杂计算
    const int iife = [] { return 6 * 7; }();
    SHOW("[]{" "}() 立即调用", iife);
    CHECK(iife == 42);

    // 捕获默认值 + 显式例外：[=, &b] 是合法的（[=, a] 才非法，因为 a 已被默认值捕获）
    int c = 100;
    auto mixed = [=, &b] { return a + b + c; };
    CHECK(mixed() == 1 + 12 + 100);
    std::cout << "  [info] [=, &b] 合法；[=, a] / [&, &b] 非法（重复捕获）\n";
}

// ---------------------------------------------------------------------------
// 【2】值捕获是 const 的：mutable 改的是 operator() 的 const 限定
//      编译期就能证明：非 mutable → const 对象可调用；mutable → const 对象不可调用
// ---------------------------------------------------------------------------
static void t2_mutable_semantics() {
    section("【2】值捕获是 const 的，mutable 改的是调用运算符的 const 限定");

    int counter = 0;
    auto noMut = [counter]() { return counter; };              // operator() const
    auto withMut = [counter]() mutable { return ++counter; };  // operator() 非 const

    static_assert(std::is_invocable_v<const decltype(noMut)&>,
                  "非 mutable lambda 的 operator() 是 const 的 → const 对象也能调用");
    static_assert(!std::is_invocable_v<const decltype(withMut)&>,
                  "mutable lambda 的 operator() 不是 const → const 对象调用不了");
    std::cout << "  [info] 编译器视角：mutable 等价于把 operator() 的 const 去掉\n";

    CHECK(withMut() == 1);
    CHECK(withMut() == 2);
    CHECK(withMut() == 3);
    SHOW("调用三次 mutable lambda 后", withMut());
    SHOW("外部 counter（没变）", counter);
    CHECK(counter == 0);   // ★ mutable 改的是 lambda 内部的副本
    std::cout << "  [info] mutable 改的是闭包对象内部的那份拷贝，外部变量纹丝不动\n";
}

// ---------------------------------------------------------------------------
// 【3】每个 lambda 表达式都是"独一无二的类型"；闭包的大小 = 捕获到的成员大小
// ---------------------------------------------------------------------------
static void t3_unique_type_and_size() {
    section("【3】每个 lambda 是唯一类型；闭包大小 = 捕获成员大小");

    auto l1 = [x = 1]() { return 1; };
    auto l2 = [x = 1]() { return 1; };          // 长得一模一样
    static_assert(!std::is_same_v<decltype(l1), decltype(l2)>,
                  "两个文本相同的 lambda 也是不同类型 → 不能放进同一个 map 的 key 而不给比较器");
    std::cout << "  [info] decltype(l1) 与 decltype(l2) 是不同类型（各有各的闭包类）\n";

    auto empty = []() {};
    SHOW("sizeof(无捕获 lambda)", sizeof(empty));
    CHECK(sizeof(empty) == 1);   // 空类也占 1 字节

    int big[4] = {1, 2, 3, 4};
    auto byVal = [big]() { return big[0]; };   // 值捕获：整个数组拷进闭包
    auto byRef = [&big]() { return big[0]; };  // 引用捕获：闭包里存的是一个指针
    SHOW("sizeof([big] 值捕获 4 个 int)", sizeof(byVal));
    SHOW("sizeof([&big] 引用捕获)", sizeof(byRef));
    CHECK(sizeof(byVal) == sizeof(big));
    CHECK(sizeof(byRef) == sizeof(int*));

    // 无捕获 lambda 能隐式转成函数指针（因为它就是一个空类 + 转换运算符）
    static_assert(std::is_convertible_v<decltype(empty), void (*)()>,
                  "无捕获 lambda 可以转成函数指针");
    static_assert(!std::is_convertible_v<decltype(byVal), int (*)()>,
                  "有捕获的 lambda 不能转成函数指针");
    void (*fp)() = [] { std::cout << "  [info] 无捕获 lambda 已转成函数指针并调用\n"; };
    fp();
    std::cout << "  [info] 这就是“能当 C 回调用”的原因；一旦有捕获就没有这个转换\n";
}

// ---------------------------------------------------------------------------
// 【4】当 日 自 检：写出"编译器视角"的等价仿函数类
// ---------------------------------------------------------------------------
namespace {

// lambda: [threshold](int v) { return v > threshold; }
struct CompareGreater {                  // ← 编译器生成的闭包类
    int threshold;                       // ← 捕获的变量成为【数据成员】
    explicit CompareGreater(int t) : threshold(t) {}   // ← 构造函数（lambda 的捕获初始化）
    bool operator()(int v) const {       // ← 非 mutable → operator() 是 const
        return v > threshold;
    }
};

// lambda: [n = 0]() mutable { return ++n; }
struct Counter {
    int n;
    Counter() : n(0) {}
    int operator()() {                   // ← mutable → operator() 没有 const
        return ++n;
    }
};

}  // namespace

static void t4_closure_equivalence() {
    section("【4】当日自检：lambda 的编译器视角等价类");

    const int threshold = 10;
    auto lam = [threshold](int v) { return v > threshold; };
    CompareGreater fun(threshold);

    // ① 体积一致：闭包里的成员 = 手写类的成员
    static_assert(sizeof(decltype(lam)) == sizeof(CompareGreater));
    // ② const 限定一致
    static_assert(std::is_invocable_v<const decltype(lam)&, int>);
    static_assert(std::is_invocable_v<const CompareGreater&, int>);

    bool same = true;
    for (int v : {-5, 10, 11, 999}) {
        if (lam(v) != fun(v)) same = false;
    }
    CHECK(same);
    std::cout << "  [info] lambda 与手写仿函数：体积相同、结果相同、const 限定相同\n";

    // mutable 版本 ↔ operator() 非 const 的类
    auto lamC = [n = 0]() mutable { return ++n; };
    Counter funC;
    static_assert(sizeof(decltype(lamC)) == sizeof(Counter));
    static_assert(!std::is_invocable_v<const Counter&>);
    CHECK(lamC() == funC());
    CHECK(lamC() == funC());
    CHECK(lamC() == funC());
    std::cout << "  [info] 三次调用后两者都是 3 → 闭包对象自己持有那份可变副本\n";
    std::cout << "  → 结论：lambda 就是【匿名类 + operator()】，捕获 = 成员，调用 = operator()\n";
}

// ---------------------------------------------------------------------------
// 【5】捕获发生在"定义处"，值捕获是那一刻的快照
// ---------------------------------------------------------------------------
static void t5_capture_is_snapshot() {
    section("【5】值捕获是定义那一刻的快照，引用捕获是别名");

    int v = 1;
    auto snapshot = [v] { return v; };
    auto alias = [&v] { return v; };
    v = 99;
    SHOW("值捕获 snapshot()", snapshot());
    SHOW("引用捕获 alias()", alias());
    CHECK(snapshot() == 1);
    CHECK(alias() == 99);
    std::cout << "  [info] 这就是“回调要值捕获”的根本原因之一：值的生命周期由闭包自己保证\n";
}

// ---------------------------------------------------------------------------
// 【6】提前看一眼周二的坑：成员函数里 [=] 捕获的是 this 指针，不是成员副本
// ---------------------------------------------------------------------------
struct Widget {
    int v = 7;

    auto byCopyDefault() {                 // [=] 在成员函数里 = 隐式捕获 this
        return [this] { return v; };       // 写成 [=] 也等价，但 C++20 已弃用隐式捕获 this
    }                                      // （真实警告见 day1_deprecated_this_warning.txt）
    auto byStarThis() {                    // [*this]（C++17）
        return [*this] { return v; };      // ← 拷贝整个对象（sizeof == sizeof(Widget)）
    }
};

static void t6_this_capture_preview() {
    section("【6】预告周二的坑：[=] 在成员函数里捕获 this（可编译期证明）");

    static_assert(
        sizeof(decltype(std::declval<Widget&>().byCopyDefault())) == sizeof(Widget*),
        "[=] 只捕获了 this 指针");
    static_assert(
        sizeof(decltype(std::declval<Widget&>().byStarThis())) == sizeof(Widget),
        "[*this] 拷贝了整个对象");

    SHOW("sizeof([this] 闭包，与 [=] 等价)", sizeof(decltype(std::declval<Widget&>().byCopyDefault())));
    SHOW("sizeof([*this] 闭包)", sizeof(decltype(std::declval<Widget&>().byStarThis())));
    SHOW("sizeof(Widget)", sizeof(Widget));

    auto w = std::make_unique<Widget>();
    auto cb = w->byCopyDefault();     // 闭包只记住 this
    w.reset();                        // 对象销毁 → cb 内部是悬垂指针（周二详解）
    std::cout << "  [info] 此时 cb 悬垂：它只记住了 this，对象已经没了（周二实机复现）\n";
    (void)cb;
}

// ---------------------------------------------------------------------------
// 【7】返回类型推导：多种返回类型必须一致，否则要显式写 -> T
// ---------------------------------------------------------------------------
static void t7_return_type() {
    section("【7】返回类型的推导与显式指定");

    auto deduced = [](int x) { return x + 1; };                 // 推导出 int
    static_assert(std::is_same_v<decltype(deduced(1)), int>);

    auto multiple = [](int x) { if (x > 0) return 1; return 0; };  // 多个 return，类型一致 → OK
    static_assert(std::is_same_v<decltype(multiple(1)), int>);

    auto explicitRet = [](int x) -> double { return x / 2.0; };    // 需要小数就显式写
    static_assert(std::is_same_v<decltype(explicitRet(1)), double>);
    SHOW("explicitRet(3)", explicitRet(3));

    auto alwaysDouble = [](int x) -> double { if (x > 0) return 1; return 0.5; };
    SHOW("alwaysDouble(1)", alwaysDouble(1));   // 隐式转 double
    std::cout << "  [info] 若多个 return 语句推导出不同类型 → 编译错误，必须写 -> T\n"
                 "         （真实报错见 day1_lambda_type_error.txt）\n";
}

// ---------------------------------------------------------------------------
// 【8】泛型 lambda（C++14）与模板 lambda（C++20，知道即可）
// ---------------------------------------------------------------------------
static void t8_generic_lambda() {
    section("【8】泛型 lambda（C++14）与模板 lambda（C++20）");

    auto generic = [](auto x) { return x + x; };   // 等价于 operator() 是模板
    SHOW("generic(21)", generic(21));
    SHOW("generic(string(\"ab\"))", generic(std::string("ab")));
    CHECK(generic(21) == 42);
    CHECK(generic(std::string("ab")) == "abab");

    auto templated = []<class T>(T x) { return x * x; };   // C++20：显式模板参数
    SHOW("templated(6)", templated(6));
    CHECK(templated(6) == 36);
    std::cout << "  [info] 泛型 lambda 的 operator() 本身是个模板，每个实参类型一份实例化\n";
}

int main() {
    std::cout << "W3 · 第一天（周一）· lambda 语法与本质\n";
    std::cout << "编译器: " << __VERSION__ << "  __cplusplus=" << __cplusplus << '\n';

    t1_syntax_anatomy();
    t2_mutable_semantics();
    t3_unique_type_and_size();
    t4_closure_equivalence();
    t5_capture_is_snapshot();
    t6_this_capture_preview();
    t7_return_type();
    t8_generic_lambda();

    std::cout << "\n-------- 结果: " << g_pass << " 通过 / " << g_fail << " 失败 --------\n";
    return g_fail == 0 ? 0 : 1;
}
