// 周三 · std::function：能存什么、SBO 阈值、手写类型擦除
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <memory>
#include <new>
#include <utility>

namespace {

std::size_t g_allocs = 0;
int free_increment(int x) { return x + 1; }

struct Functor {
    int k;
    int operator()(int x) const { return x + k; }
};

struct Widget {
    int v;
    int get(int x) const { return x + v; }
};

}  // namespace

// 替换全局 operator new，统计堆分配次数（只在本示例里用）
void* operator new(std::size_t size) {
    ++g_allocs;
    if (void* p = std::malloc(size)) {
        return p;
    }
    throw std::bad_alloc();
}
void operator delete(void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void* operator new[](std::size_t size) { return ::operator new(size); }
void operator delete[](void* p) noexcept { ::operator delete(p); }
void operator delete[](void* p, std::size_t) noexcept { ::operator delete(p); }

namespace {

// 捕获一个 N 字节的对象，看 std::function 会不会去堆上分配
template <std::size_t N>
struct Payload {
    char bytes[N];
};

template <std::size_t N>
std::size_t allocsWhenCapturing() {
    g_allocs = 0;
    std::function<void()> fn = [payload = Payload<N>{}]() mutable { payload.bytes[0] = 1; };
    std::size_t allocs = g_allocs;
    fn();
    return allocs;
}

// 手写一个 std::function：抽象基类 + 模板派生 + 指针（类型擦除三件套）
class MiniFunction {
public:
    template <class F>
    explicit MiniFunction(F f) : p_(std::make_unique<Model<F>>(std::move(f))) {}

    MiniFunction(MiniFunction&&) noexcept = default;
    MiniFunction& operator=(MiniFunction&&) noexcept = default;
    MiniFunction(const MiniFunction&) = delete;
    MiniFunction& operator=(const MiniFunction&) = delete;

    void operator()() const { p_->call(); }

private:
    struct Concept {
        virtual ~Concept() = default;
        virtual void call() = 0;
    };

    template <class F>
    struct Model final : Concept {
        explicit Model(F f) : f_(std::move(f)) {}
        void call() override { f_(); }
        F f_;
    };

    std::unique_ptr<Concept> p_;
};

}  // namespace

int main() {
    std::printf("sizeof(std::function<void()>)    = %zu 字节\n", sizeof(std::function<void()>));
    std::printf("sizeof(std::function<int(int)>)  = %zu 字节（与签名无关，固定开销）\n",
                sizeof(std::function<int(int)>));
    std::printf("sizeof(MiniFunction)             = %zu 字节（手写版只要一个指针）\n",
                sizeof(MiniFunction));
    std::printf("sizeof(函数指针)                 = %zu 字节\n", sizeof(void (*)()));

    std::printf("\n[SBO 阈值扫描] libstdc++：捕获 ≤16 字节放对象内部，>16 字节走堆分配\n");
    std::printf("  捕获 8  字节 → %zu 次堆分配\n", allocsWhenCapturing<8>());
    std::printf("  捕获 16 字节 → %zu 次堆分配\n", allocsWhenCapturing<16>());
    std::printf("  捕获 17 字节 → %zu 次堆分配\n", allocsWhenCapturing<17>());
    std::printf("  捕获 32 字节 → %zu 次堆分配\n", allocsWhenCapturing<32>());

    std::printf("\n[能存哪些可调用对象]\n");
    std::function<int(int)> f1 = [](int x) { return x + 1; };            // 无捕获 lambda
    int base = 10;
    std::function<int(int)> f2 = [base](int x) { return x + base; };     // 有捕获 lambda
    std::function<int(int)> f3 = &free_increment;                        // 函数指针
    std::function<int(int)> f4 = Functor{5};                             // 仿函数
    Widget w{100};
    std::function<int(int)> f5 = [&w](int x) { return w.get(x); };       // 成员函数 → lambda
    std::printf("  f1(1)=%d  f2(1)=%d  f3(1)=%d  f4(1)=%d  f5(1)=%d\n",
                f1(1), f2(1), f3(1), f4(1), f5(1));

    MiniFunction mini([]{ std::printf("\n[MiniFunction] 手写类型擦除也能调用\n"); });
    mini();

    return 0;
}
