#include "test_assert.hpp"
#include "w3/scope_guard.hpp"

#include <stdexcept>
#include <utility>

int main() {
    using w3::onExit;
    using w3::ScopeGuard;

    // 1) 出作用域触发一次
    {
        int hits = 0;
        {
            auto guard = onExit([&hits] { ++hits; });
            CHECK(guard.active());
            CHECK(hits == 0);
        }
        CHECK(hits == 1);
    }

    // 2) dismiss 后不触发
    {
        int hits = 0;
        {
            auto guard = onExit([&hits] { ++hits; });
            guard.dismiss();
            CHECK(!guard.active());
        }
        CHECK(hits == 0);
    }

    // 3) 移动后源置空：只清理一次（否则就是 double-close）
    {
        int hits = 0;
        {
            ScopeGuard a = onExit([&hits] { ++hits; });
            ScopeGuard b = std::move(a);
            CHECK(!a.active());   // 源被置空
            CHECK(b.active());
        }
        CHECK(hits == 1);
    }

    // 4) 异常退出路径同样清理
    {
        int hits = 0;
        try {
            auto guard = onExit([&hits] { ++hits; });
            throw std::runtime_error("boom");
        } catch (const std::exception&) {
            // 忽略
        }
        CHECK(hits == 1);
    }

    // 5) 多个守卫按构造的逆序析构
    {
        int hits = 0;
        {
            auto first = onExit([&hits] { hits += 1; });
            auto second = onExit([&hits] { hits += 10; });
        }
        CHECK(hits == 11);   // 逆序：second(+=10) 先，first(+=1) 后
    }

    return w3test::summary("scope_guard");
}
