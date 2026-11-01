#pragma once

// =============================================================================
// ScopeGuard —— RAII + lambda
//
// 把"必须成对的操作"（open/close、lock/unlock、malloc/free）交给析构函数。
// 和 W2 的 FileDescriptor 同一个思想，只是清理动作换成了任意 lambda。
// =============================================================================

#include <functional>
#include <utility>

namespace w3 {

class [[nodiscard]] ScopeGuard {
public:
    using Cleanup = std::function<void()>;

    explicit ScopeGuard(Cleanup cleanup) : cleanup_(std::move(cleanup)) {}

    // 守卫本身不能拷贝：拷贝会让"谁负责清理"变得不唯一
    ScopeGuard(const ScopeGuard&) = delete;
    ScopeGuard& operator=(const ScopeGuard&) = delete;

    // 移动：所有权转给新对象，源置为 inactive
    // 不置空的话，源析构时会再执行一次清理 —— 就是 double-close 那个 bug
    ScopeGuard(ScopeGuard&& other) noexcept
        : cleanup_(std::move(other.cleanup_)),
          active_(std::exchange(other.active_, false)) {}

    ScopeGuard& operator=(ScopeGuard&&) = delete;

    ~ScopeGuard() {
        if (active_ && cleanup_) {
            cleanup_();
        }
    }

    // 主动放弃清理（对应 W2 的 file_descriptor.release()）
    void dismiss() noexcept { active_ = false; }

    [[nodiscard]] bool active() const noexcept { return active_; }

private:
    Cleanup cleanup_;
    bool active_ = true;
};

// onExit(f) 比手写 ScopeGuard(f) 更贴最近被引入的 std::experimental::scope_exit
[[nodiscard]] inline ScopeGuard onExit(ScopeGuard::Cleanup cleanup) {
    return ScopeGuard(std::move(cleanup));
}

}  // namespace w3
