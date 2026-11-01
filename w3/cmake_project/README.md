# w3 · W3 收官 CMake 工程

一个"最小但完整"的 C++20 工程，把 W3 的三样产出放进同一套构建系统：
**lambda 回调 + `std::function` 任务管理 + ASan/UBSan 工具链**。

对应 `验收示例\W3_验收.md` 的 **C 档**（把 ASan 集成进 CMake、写 README 记录怎么抓 bug）
和 **A 档第 1 条**（`TaskRunner` 能注册并执行 ≥3 个回调）。

## 一、快速开始

```powershell
# 配置 + 构建 + 测试（一步到位）
cmake --preset debug
cmake --build --preset debug
ctest --preset debug

# 换构建类型
cmake --preset release
cmake --build --preset release
```

没有 `CMakePresets.json` 支持的旧版本，等价的手工命令：

```powershell
cmake -S . -B build/debug -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug
cmake --build build/debug -j
ctest --test-dir build/debug --output-on-failure
```

## 二、目录结构

```
cmake_project/
├── CMakeLists.txt            顶层：库 + 示例 + 测试 + 安装导出
├── CMakePresets.json         debug / release / debug-asan / thread 四个预设
├── cmake/
│   ├── Sanitizers.cmake      Debug-ASan 构建类型 + sanitizer 可用性探测
│   └── w3coreConfig.cmake.in 包配置模板（让别的项目 find_package(w3core)）
├── include/w3/
│   ├── scope_guard.hpp       RAII + lambda：出作用域自动清理
│   └── task_runner.hpp       std::function 回调队列
├── src/task_runner.cpp
├── examples/
│   ├── ex_lambda_basics.cpp  周一：闭包大小 / mutable / 仿函数等价
│   ├── ex_std_function.cpp   周三：SBO 阈值 / 手写类型擦除
│   ├── ex_task_runner.cpp    周六：6 种捕获方式各注册一个回调
│   └── ex_asan_bug.cpp       周五：故意越界 / UAF / 泄漏（不进 ctest）
└── tests/
    ├── test_assert.hpp
    ├── test_scope_guard.cpp
    └── test_task_runner.cpp
```

## 三、怎么用 ASan 抓 bug（C 档第 2 条）

### 1. 开一个带 sanitizer 的构建

```powershell
cmake --preset debug-asan
cmake --build --preset debug-asan
```

`Debug-ASan` 这个构建类型由 `cmake/Sanitizers.cmake` 定义，等价于：

```
-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer
```

### 2. 跑那个"故意写坏"的程序

```powershell
# Windows 上需要把 sanitizer 运行时目录加进 PATH（Linux 不用）
.\build\debug-asan\ex_asan_bug.exe oob
.\build\debug-asan\ex_asan_bug.exe uaf
.\build\debug-asan\ex_asan_bug.exe leak
```

### 3. 读报错

ASan 的报错有三段，按顺序看：

| 段 | 内容 | 你要的答案 |
| :--- | :--- | :--- |
| 第一行 | `ERROR: AddressSanitizer: stack-buffer-overflow` | **bug 类型**（越界 / UAF / 泄漏） |
| 第一段栈 | `#0 ... #1 ...` | **在哪写坏的**（出事的指令） |
| 第二段栈 | `allocated by thread T0 here` / `previously allocated by` | **这块内存是谁分配的**（越界通常就是它的邻居） |

`SUMMARY: AddressSanitizer: stack-buffer-overflow ... in main` 那一行是给 CI 抓取的。

### 4. 泄漏检测要显式开

```powershell
$env:ASAN_OPTIONS = "detect_leaks=1"
```

Linux/macOS 默认开，Windows 上默认关。本工程的 ctest 已经用
`set_tests_properties(... ENVIRONMENT "ASAN_OPTIONS=detect_leaks=1")` 挂好了。

### 5. 什么时候**不要**开

- Release 构建：ASan 有数倍的性能/内存开销，压测数据会失真；
- 发布包：sanitizer 运行时不是产品依赖；
- `-O2` 调试：行号会漂、变量可能被优化掉，定位用 `-O0 -g`（本工程的 DebugASan 用 `-O1` 折中）。

## 四、本机限制（重要）

**MinGW-w64 的 GCC 不附带 libasan**，链接时报 `ld: cannot find -lasan`。
所以 `cmake --preset debug-asan` 在本机会**在配置阶段就失败**，并给出三条路径：

1. 用 WSL2 / Linux 上的 GCC 或 Clang 构建（推荐，周五实验本来就按 Linux 写）；
2. 用 MSVC 的 `/fsanitize=address`（VS 2019 16.9+）；
3. 只做普通 `Debug` 构建。

这不是缺陷，而是 `Sanitizers.cmake` 里 `check_cxx_source_compiles` 探测的**预期结果**——
错误在配置期暴露，而不是等到链接期才给你一句 `cannot find -lasan`。

## 五、这个工程包含的工程化能力

| # | 能力 | 在哪 |
| :---: | :--- | :--- |
| 1 | C++20、禁编译器扩展 | `CMakeLists.txt` 全局设置 |
| 2 | 静态库 + PUBLIC 头文件路径 | `add_library(w3core STATIC ...)` |
| 3 | 编译告警统一（GCC/Clang、MSVC 分支） | `w3_set_warnings()` |
| 4 | 警告即错误开关 | `-DW3_WERROR=ON` |
| 5 | 自定义构建类型 `DebugASan` | `cmake/Sanitizers.cmake` |
| 6 | sanitizer 可用性探测（配置期失败） | `check_cxx_source_compiles` |
| 7 | CTest 测试 + 环境变量 | `w3_add_test()` |
| 8 | 安装规则与包导出（`find_package(w3core)`） | `install(EXPORT ...)` |
| 9 | 四个预设一键跑 | `CMakePresets.json` |
| 10 | `compile_commands.json` 给 clangd | `CMAKE_EXPORT_COMPILE_COMMANDS` |
