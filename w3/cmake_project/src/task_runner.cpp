#include "w3/task_runner.hpp"

#include <cstddef>
#include <utility>

namespace w3 {

std::size_t TaskRunner::add(Task task) {
    tasks_.push_back(std::move(task));
    names_.push_back("<unnamed>");
    return tasks_.size();
}

std::size_t TaskRunner::add(std::string name, Task task) {
    tasks_.push_back(std::move(task));
    names_.push_back(std::move(name));
    return tasks_.size();
}

std::size_t TaskRunner::runAll() {
    std::size_t executed = 0;
    for (auto& task : tasks_) {
        if (task) {
            task();
            ++executed;
        }
    }
    return executed;
}

void TaskRunner::clear() noexcept {
    tasks_.clear();
    names_.clear();
}

}  // namespace w3
