// =============================================================================
//  W3 · 第三天（周三）· std::function：类型擦除与代价
//
//  验收页周三任务：std::function：类型擦除、能存什么；std::bind 基本被 lambda 取代
//  当日自检：能说清 std::function 为什么有性能开销
//
//  本文件把"为什么有开销"拆成可测量的四件事：
//    【1】sizeof 与"能存什么"（含成员函数指针、泛型 lambda、bind 结果）
//    【2】代价①：捕获多大就走堆分配（SBO 阈值扫描）—— 数全局 operator new
//    【3】代价②：调用开销（直接 lambda / 函数指针 / std::function 对比）
//    【4】手写 MiniFunction —— 看清"类型擦除"长什么样，并证明标准库的 SBO 真在省分配
//    【5】行为要点：mutable lambda、空 function 调用、拷贝语义
//    【6】std::bind vs lambda（含 std::ref 陷阱）
//
//  编译： g++ -std=c++20 -Wall -Wextra -O2 day3_std_function.cpp -o day3_O2.exe
//         g++ -std=c++20 -Wall -Wextra -O0 day3_std_function.cpp -o day3_O0.exe
// =============================================================================
#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <new>
#include <string>
#include <type_traits>
#include <utility>

// ---------------------------------------------------------------------------
//  统计全局 new/delete：用来数 std::function 到底分配了几次
// ---------------------------------------------------------------------------
static long g_allocs = 0;

void* operator new(std::size_t n) {
    ++g_allocs;
    if (void* p = std::malloc(n ? n : 1)) return p;
    throw std::bad_alloc();
}
void* operator new[](std::size_t n) { return ::operator new(n); }
void  operator delete(void* p) noexcept { std::free(p); }
void  operator delete[](void* p) noexcept { std::free(p); }
void  operator delete(void* p, std::size_t) noexcept { std::free(p); }
void  operator delete[](void* p, std::size_t) noexcept { std::free(p); }

// ---------------------------------------------------------------------------
static int g_pass = 0;
static int g_fail = 0;
#define CHECK(expr)                                                              \
    do {                                                                         \
        if (expr) { ++g_pass; }                                                  \
        else { ++g_fail; std::cout << "  [FAIL] line " << __LINE__ << ": " #expr "\n"; } \
    } while (0)
static void section(const char* t) { std::cout << "\n== " << t << " ==\n"; }
static void ok(const std::string& m) { std::cout << "  [ ok ] " << m << '\n'; }

// ---------------------------------------------------------------------------
//  可调用对象的几种形态
// ---------------------------------------------------------------------------
static int freeFunc(int x) { return x + 1000; }          // ① 自由函数

struct Functor {                                          // ② 仿函数
    int bias = 7;
    int operator()(int x) const { return x + bias; }
};

struct Widget {                                           // 成员函数指针用
    int v = 5;
    int value() const { return v; }
};

// ===========================================================================
//  【1】sizeof 与"能存什么"
// ===========================================================================
static void t1_what_can_it_store() {
    section("【1】std::function 的 sizeof 与能存什么");

    std::cout << "  [info] sizeof(std::function<void()>)   = " << sizeof(std::function<void()>)
              << " 字节\n";
    std::cout << "  [info] sizeof(std::function<int(int)>) = " << sizeof(std::function<int(int)>)
              << " 字节\n";
    std::cout << "  [info] sizeof(void(*)())               = " << sizeof(void (*)()) << " 字节\n";
    std::cout << "  [info] 无捕获的 lambda 本身只有 1 字节，装进 std::function 变成 "
              << sizeof(std::function<void()>) << " 字节 —— 这就是类型擦除的存储成本\n";
    CHECK(sizeof(std::function<void()>) > sizeof(void (*)()));

    std::function<int(int)> f1 = &freeFunc;                            // ① 自由函数指针
    CHECK(f1(1) == 1001);

    std::function<int(int)> f2 = [](int x) { return x * 2; };          // ② 无捕获 lambda
    CHECK(f2(21) == 42);

    int base = 100;                                                    // ③ 有捕获 lambda
    std::function<int(int)> f3 = [base](int x) { return x + base; };
    CHECK(f3(1) == 101);

    std::function<int(int)> f4 = Functor{};                            // ④ 仿函数
    CHECK(f4(1) == 8);

    Widget w;
    std::function<int(const Widget&)> f5 = &Widget::value;             // ⑤ 成员函数指针
    CHECK(f5(w) == 5);                                                 //    INVOKE：对象作第一个实参
    std::function<int(const Widget&)> f5b = [](const Widget& x) { return x.value(); };
    CHECK(f5b(w) == 5);                                                //    推荐：lambda 包一层更清楚

    std::function<int(int)> f6 = std::bind(freeFunc, std::placeholders::_1);  // ⑥ bind 结果
    CHECK(f6(1) == 1001);

    std::function<int(int)> f7 = f2;                                   // ⑦ 另一个 std::function
    CHECK(f7(21) == 42);

    std::function<int(int)> f8 = [](auto x) { return static_cast<int>(x) * 3; };  // ⑧ 泛型 lambda
    CHECK(f8(2) == 6);

    ok("能存：自由函数指针 / 无捕获 lambda / 有捕获 lambda / 仿函数 / 成员函数指针 / bind 结果 / 另一个 function / 泛型 lambda");
}

// ===========================================================================
//  【2】代价①：捕获多大就走堆分配
// ===========================================================================
template <std::size_t N>
struct Blob {
    char data[N] = {};
};

template <std::size_t N>
static long allocs_for_capture() {
    Blob<N> b{};
    long before = g_allocs;
    {
        std::function<void()> f = [b]() { (void)b; };   // 值捕获一个 N 字节的对象
        if (f) f();
    }
    return g_allocs - before;
}

static void t2_allocation_threshold() {
    section("【2】代价①：小捕获不分配（SBO），超过阈值就堆分配");
    std::cout << "  [说明] 阈值标准未规定，以下是 libstdc++ 的实测扫描：\n";

    for (std::size_t n : {std::size_t{1}, std::size_t{8}, std::size_t{16}, std::size_t{17},
                          std::size_t{24}, std::size_t{32}, std::size_t{64}}) {
        long used = 0;
        switch (n) {
            case 1:  used = allocs_for_capture<1>();  break;
            case 8:  used = allocs_for_capture<8>();  break;
            case 16: used = allocs_for_capture<16>(); break;
            case 17: used = allocs_for_capture<17>(); break;
            case 24: used = allocs_for_capture<24>(); break;
            case 32: used = allocs_for_capture<32>(); break;
            default: used = allocs_for_capture<64>(); break;
        }
        std::cout << "  [info] 捕获 " << n << " 字节 → 堆分配 " << used << " 次\n";
    }

    CHECK(allocs_for_capture<16>() == 0);   // 小对象：内联缓冲，0 次分配
    CHECK(allocs_for_capture<64>() == 1);   // 大对象：1 次堆分配
    std::cout << "  [info] 结论：libstdc++ 下 ≤16 字节走内联缓冲（SBO），>16 字节堆分配\n";
    std::cout << "  [info] 但这是实现定义行为，换标准库/编译器阈值会变 —— 不要依赖具体数字\n";
    ok("代价①：小捕获靠 SBO 免分配；大捕获（string、大数组）每次构造都要 new");
}

// ===========================================================================
//  【3】代价②：调用开销（能否内联）
// ===========================================================================
static long g_bench_sink = 0;
[[gnu::noinline]] static void keep(long v) { g_bench_sink ^= v; }

// 用 noinline 工厂把"目标是谁"藏起来，防止编译器把间接调用去虚拟化后内联 —— 否则测不准
[[gnu::noinline]] static long (*make_fnptr())(long) {
    return [](long x) { return x + 1; };
}
[[gnu::noinline]] static std::function<long(long)> make_std_fn() {
    return [](long x) { return x + 1; };
}

template <class F>
static double ns_per_call(F&& f, long iters) {
    double best = 1e18;
    for (int round = 0; round < 3; ++round) {
        long acc = 0;
        auto t0 = std::chrono::steady_clock::now();
        for (long i = 0; i < iters; ++i) acc += f(i);
        auto t1 = std::chrono::steady_clock::now();
        keep(acc);
        double ns = std::chrono::duration<double, std::nano>(t1 - t0).count() /
                    static_cast<double>(iters);
        if (ns < best) best = ns;
    }
    return best;
}

static void t3_call_overhead() {
    section("【3】代价②：直接调用 / 函数指针 / std::function 的调用开销");
    constexpr long kIters = 20'000'000;

    auto direct = [](long x) { return x + 1; };      // 编译器能内联
    auto fp = make_fnptr();                          // 只能间接调用
    auto sf = make_std_fn();                         // 类型擦除 + 间接

    double d1 = ns_per_call(direct, kIters);
    double d2 = ns_per_call(fp, kIters);
    double d3 = ns_per_call(sf, kIters);

    std::cout << "  [info] 直接 lambda   : " << d1 << " ns/次\n";
    std::cout << "  [info] 函数指针      : " << d2 << " ns/次   （×" << (d2 / d1) << "）\n";
    std::cout << "  [info] std::function : " << d3 << " ns/次   （×" << (d3 / d1) << "）\n";
#ifdef __OPTIMIZE__
    std::cout << "  [info] 本程序是优化构建：能内联的直接调用被内联，差距明显\n";
#else
    std::cout << "  [info] 本程序是 -O0 构建：都不内联，三者接近 —— 说明开销主要来自"
                 "「能否内联」，而不是「多了一次函数调用」\n";
#endif
    std::cout << "  [info] 微基准只用来说明量级与机制，绝对值随机器变化\n";
    CHECK(d1 > 0 && d2 > 0 && d3 > 0);
    ok("代价②：擦除后只能间接调用、无法内联；与函数指针同量级，但比可内联的直接调用慢数倍");
}

// ===========================================================================
//  【4】手写 MiniFunction：类型擦除长什么样
// ===========================================================================
class MiniFunction {                       // 极简版：只支持 void()
    struct Concept {                       // "概念"接口 = 类型擦除的边界
        virtual ~Concept() = default;      // 要通过基类指针删除 → 必须有虚析构
        virtual void call() = 0;
        virtual std::unique_ptr<Concept> clone() const = 0;
    };
    template <class F>
    struct Model final : Concept {         // 把"具体类型"藏在这里
        F f;
        explicit Model(F fn) : f(std::move(fn)) {}
        void call() override { f(); }
        std::unique_ptr<Concept> clone() const override { return std::make_unique<Model>(f); }
    };
    std::unique_ptr<Concept> p_;           // ← 被擦除之后，只剩这一个指针

public:
    template <class F>
    MiniFunction(F fn) : p_(std::make_unique<Model<F>>(std::move(fn))) {}   // 源码上是 1 次堆分配
    MiniFunction(const MiniFunction& o) : p_(o.p_ ? o.p_->clone() : nullptr) {}
    MiniFunction& operator=(const MiniFunction& o) {
        if (this != &o) p_ = o.p_ ? o.p_->clone() : nullptr;
        return *this;
    }
    MiniFunction(MiniFunction&&) noexcept = default;
    MiniFunction& operator=(MiniFunction&&) noexcept = default;

    void operator()() {
        if (!p_) throw std::bad_function_call();   // 空调用：与标准库一致
        p_->call();
    }
    explicit operator bool() const noexcept { return p_ != nullptr; }
};

// noinline 工厂：让"那次分配"真的发生在函数里，编译器无法把它优化掉
[[gnu::noinline]] static MiniFunction make_mini_with_hit(int& hits) {
    return MiniFunction([&hits] { ++hits; });
}
[[gnu::noinline]] static std::function<void()> make_stdfn_with_hit(int& hits) {
    return std::function<void()>([&hits] { ++hits; });
}

static void t4_type_erasure_by_hand() {
    section("【4】手写 MiniFunction：看清类型擦除，并证明标准库的 SBO 真在省分配");
    std::cout << "  [info] 机制：抽象基类 Concept + 模板派生 Model<F> + 指针持有\n";
    std::cout << "  [info] 代价：一次堆分配 + 一次虚调用，并且彻底丢掉具体类型\n";
    std::cout << "  [info] 注意：-O2 下 GCC 会做 allocation elision —— 即使把分配放进 noinline 工厂、"
                 "让地址写进 volatile 全局，也照样被优化掉；所以两种优化级别各存一份输出对照\n";

    int hits = 0;
    long before = g_allocs;
    MiniFunction mini = make_mini_with_hit(hits);          // 工厂内必然 new 一次
    long miniAllocs = g_allocs - before;

    before = g_allocs;
    std::function<void()> stdFn = make_stdfn_with_hit(hits);   // 小捕获 → SBO
    long stdAllocs = g_allocs - before;

    mini();
    stdFn();
    CHECK(hits == 2);
    std::cout << "  [info] 同样捕获 1 个指针：MiniFunction 分配 " << miniAllocs
              << " 次，std::function 分配 " << stdAllocs << " 次\n";
#ifdef __OPTIMIZE__
    std::cout << "  [info] -O2 下 GCC 甚至把手写版那次 new 也优化掉了：证明对象不逃逸后改用栈存储\n"
                 "         （allocation elision，N3664 允许；连“把地址写进 volatile 全局”都拦不住）\n"
                 "  [info] 想看类型擦除“必然分配”的干净数字，请看 -O0 那份输出（1 次 vs 0 次）\n";
#else
    std::cout << "  [info] -O0 下没有优化干扰：手写版必然 1 次堆分配，标准库靠 SBO 省成 0 次\n";
#endif
    CHECK(stdAllocs == 0);                  // 小捕获走 SBO：两个优化级别下都成立
    CHECK(miniAllocs >= stdAllocs);         // 手写版不会比标准库更省（-O0 下是 1 vs 0）

    std::cout << "  [info] sizeof(MiniFunction)=" << sizeof(MiniFunction)
              << "  sizeof(std::function<void()>)=" << sizeof(std::function<void()>)
              << "  ← 标准库更大：它自带内联缓冲区（用空间换掉那次分配）\n";

    // 空 function：移动走之后源为空，调用要抛
    MiniFunction src = [] {};
    MiniFunction dst = std::move(src);
    CHECK(!src);
    CHECK(static_cast<bool>(dst));
    bool miniThrew = false;
    try { src(); } catch (const std::bad_function_call&) { miniThrew = true; }
    CHECK(miniThrew);
    dst();
    ok("类型擦除 = Concept/Model + 一个指针；标准库用 SBO 把小对象的那次分配省掉了");
}

// ===========================================================================
//  【5】行为要点
// ===========================================================================
static void t5_behaviors() {
    section("【5】行为要点：mutable lambda、空 function、拷贝语义");

    std::function<int()> counter = [n = 0]() mutable { return ++n; };
    CHECK(counter() == 1);
    CHECK(counter() == 2);
    CHECK(counter() == 3);
    std::cout << "  [info] mutable lambda 装进 std::function 后，状态存在 function 内部：三次调用 = 1,2,3\n";

    const std::function<int()>& cref = counter;      // 即使 wrapper 是 const 也能调用
    CHECK(cref() == 4);
    std::cout << "  [info] const std::function 也能调用 non-const 的 target：operator() 是 const，"
                 "但被擦除的目标按非 const 调用\n";

    std::function<int()> copy1 = [n = 100]() mutable { return ++n; };
    std::function<int()> copy2 = copy1;             // 拷贝 = 拷贝一份 target
    CHECK(copy1() == 101);
    CHECK(copy2() == 101);                          // 各自从 100 开始 → 状态独立
    CHECK(copy1() == 102);
    std::cout << "  [info] 拷贝 std::function = 拷贝一份 target，两份状态互相独立\n";

    std::function<void()> empty;
    CHECK(!empty);                                  // 用前判空
    CHECK(empty == nullptr);
    bool threw = false;
    try { empty(); } catch (const std::bad_function_call& e) {
        threw = true;
        std::cout << "  [info] 空调用抛出: " << e.what() << '\n';
    }
    CHECK(threw);
    ok("三件事：mutable 状态在内部 / 空调用抛 bad_function_call（先判空）/ 拷贝会拷贝 target");
}

// ===========================================================================
//  【6】std::bind vs lambda
// ===========================================================================
static void incRef(int& x) { ++x; }

static void t6_bind_vs_lambda() {
    section("【6】std::bind 为什么基本被 lambda 取代");

    auto byBind = std::bind(freeFunc, std::placeholders::_1);
    auto byLambda = [](int x) { return freeFunc(x); };
    CHECK(byBind(1) == byLambda(1));
    std::cout << "  [info] 等价写法：bind 要写占位符 _1，lambda 直接写参数名（可读性）\n";

    int v1 = 0;
    auto b1 = std::bind(incRef, v1);            // ⚠️ 默认按值"存储"参数 → 改的是副本
    b1();
    std::cout << "  [info] std::bind(incRef, v1) 之后 v1 = " << v1 << "（没生效：改的是副本）\n";
    CHECK(v1 == 0);

    int v2 = 0;
    auto b2 = std::bind(incRef, std::ref(v2));  // 必须显式 std::ref 才行
    b2();
    std::cout << "  [info] std::bind(incRef, std::ref(v2)) 之后 v2 = " << v2 << "（可以了）\n";
    CHECK(v2 == 1);

    int v3 = 0;
    auto l3 = [&v3] { incRef(v3); };            // lambda：捕获方式一眼可见
    l3();
    CHECK(v3 == 1);
    std::cout << "  [info] lambda 的 [&v3] 直接说明是引用捕获，不必记住 std::ref 规则\n";

    std::cout << "  [info] 第三个理由：bind 表达式的类型几乎无法书写，报错信息极长；"
                 "lambda 语义清楚得多\n";
    ok("bind 被取代的三个理由：可读性差 / std::ref 这类隐式规则坑人 / 报错难懂");
}

// ===========================================================================
int main() {
    std::cout << "W3 · 第三天（周三）· std::function：类型擦除与代价\n";
    std::cout << "编译器: " << __VERSION__ << "  __cplusplus=" << __cplusplus << '\n';

    t1_what_can_it_store();
    t2_allocation_threshold();
    t3_call_overhead();
    t4_type_erasure_by_hand();
    t5_behaviors();
    t6_bind_vs_lambda();

    std::cout << "\n-------- 结果: " << g_pass << " 通过 / " << g_fail << " 失败 --------\n";
    return g_fail == 0 ? 0 : 1;
}
