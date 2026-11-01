// 周五 · 故意写坏的 ASan 演示：进 Debug-ASan 构建后运行，看 sanitizer 报什么
//
//   ex_asan_bug oob   越界写（stack-buffer-overflow）
//   ex_asan_bug uaf   use-after-free
//   ex_asan_bug leak  内存泄漏（需要 LeakSanitizer，Linux/macOS 默认开）
//
// 这些目标故意不进 ctest：它们"通过"的方式就是崩给 ASan 看。
#include <cstdio>
#include <cstring>

namespace {

void outOfBounds() {
    int a[3] = {0, 0, 0};
    a[5] = 1;   // 越界写：ASan 报 stack-buffer-overflow
    std::printf("a[5]=%d\n", a[5]);
}

void useAfterFree() {
    int* p = new int(42);
    delete p;
    std::printf("uaf read=%d\n", *p);   // 释放后读
}

void leak() {
    new int(7);   // 没有 delete
}

}  // namespace

int main(int argc, char** argv) {
    const char* mode = (argc > 1) ? argv[1] : "oob";

    if (std::strcmp(mode, "oob") == 0) {
        outOfBounds();
    } else if (std::strcmp(mode, "uaf") == 0) {
        useAfterFree();
    } else if (std::strcmp(mode, "leak") == 0) {
        leak();
    } else {
        std::printf("用法：%s [oob|uaf|leak]\n", argv[0]);
        return 2;
    }

    std::printf("（没有报错说明这次没开 sanitizer）\n");
    return 0;
}
