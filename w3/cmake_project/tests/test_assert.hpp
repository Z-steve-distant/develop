#pragma once

// 极简断言工具：不引入任何第三方测试框架（W1–W3 的工程保持零依赖）

#include <cstdio>

namespace w3test {

inline int g_checks = 0;
inline int g_failures = 0;

inline void check(bool ok, const char* expr, const char* file, int line) {
    ++g_checks;
    if (!ok) {
        ++g_failures;
        std::printf("[FAIL] %s:%d  %s\n", file, line, expr);
    }
}

inline int summary(const char* name) {
    std::printf("[%s] %d 项检查，%d 项失败\n", name, g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}

}  // namespace w3test

#define CHECK(cond) \
    ::w3test::check(static_cast<bool>(cond), #cond, __FILE__, __LINE__)
