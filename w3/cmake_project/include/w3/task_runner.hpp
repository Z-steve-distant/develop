#pragma once

// =============================================================================
// TaskRunner —— std::function 的典型用法：管理一批"类型各异的回调"
//
// 这是 W3 验收 A 档第 1 条要的东西：能注册并执行 ≥3 个回调。
// 为什么必须用 std::function：每个 lambda 的闭包类型都是**独一无二的**，
// 没有 std::function 就没法把它们放进同一个容器。
// =============================================================================

#include <cstddef>
#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace w3 {

class TaskRunner {
public:
    using Task = std::function<void()>;

    std::size_t add(Task task);
    std::size_t add(std::string name, Task task);

    [[nodiscard]] std::size_t size() const noexcept { return tasks_.size(); }
    [[nodiscard]] bool empty() const noexcept { return tasks_.empty(); }
    [[nodiscard]] const std::vector<std::string>& names() const noexcept { return names_; }

    // 依次执行全部任务，返回实际执行的个数（空的 std::function 会被跳过）
    std::size_t runAll();

    void clear() noexcept;

private:
    std::vector<Task> tasks_;
    std::vector<std::string> names_;
};

}  // namespace w3
