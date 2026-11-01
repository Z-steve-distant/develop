# W3 · lambda / std::function / STL 深入 / ASan

阶段一（W1–W3）收官归档，对应 `验收示例/W3_验收.md`。
整理日期：2026-10-18

## 归档件

| 文件 | 对应验收项 | 状态 |
| :--- | :--- | :--- |
| `task_runner.cpp` | A 档 1 | 通过：注册并执行 3 个回调 |
| `capture_demo.cpp` | A 档 2 | 通过：五种捕获 + 生命周期风险注释 |
| `scope_guard.hpp` | 练习 1 | 通过：禁拷贝、可移动、dismiss |
| `asan_leak_report.txt` | A 档 3 | 通过：越界 / UAF / 泄漏三份原始报错 |
| `W3讲清.m4a` | B 档 | **缺失，待重录** |
| `自评.md` | 周日验收记录 | 已填，B 档记欠账 |
| `cmake_project/` | C 档 | 通过：Debug + DebugASan，ctest 2/2 |

## 目录

```
w3/
├── task_runner.cpp            A 档 1：Linux 手写的 TaskRunner（3 个回调）
├── capture_demo.cpp           A 档 2：五种捕获方式
├── scope_guard.hpp            练习 1：RAII + lambda
├── asan_leak_report.txt       A 档 3：ASan 原始输出
├── 自评.md
├── cmake_project/             C 档：完整 CMake 工程
│   ├── include/w3/            task_runner.hpp / scope_guard.hpp
│   ├── src/  tests/  examples/
│   └── cmake/Sanitizers.cmake DebugASan 构建类型 + 工具链探测
├── daily/
│   ├── day1/                  周一：lambda 语法、报错样本、输出
│   ├── day3/                  周三：std::function 代价、move-only 探测
│   └── day5/asan_probe.cpp    周五：越界实验草稿
├── notes/第一天_讲解.md
└── _build_artifacts/          可执行文件，可随时删除
```

## 复现

```bash
# A 档 2
g++ -std=c++20 -O0 -g capture_demo.cpp -o _build_artifacts/capture_demo
./_build_artifacts/capture_demo

# C 档：普通 Debug 构建
cmake -S cmake_project -B cmake_project/build/linux-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build cmake_project/build/linux-debug -j
ctest --test-dir cmake_project/build/linux-debug --output-on-failure

# C 档：DebugASan 构建
cmake -S cmake_project -B cmake_project/build/linux-asan -DCMAKE_BUILD_TYPE=DebugASan
cmake --build cmake_project/build/linux-asan -j
ctest --test-dir cmake_project/build/linux-asan --output-on-failure

# A 档 3：重新生成报告
g++ -std=c++20 -O0 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
    cmake_project/examples/ex_asan_bug.cpp -o _build_artifacts/ex_asan_bug
ASAN_OPTIONS=detect_leaks=1 ./_build_artifacts/ex_asan_bug oob
ASAN_OPTIONS=detect_leaks=1 ./_build_artifacts/ex_asan_bug uaf
ASAN_OPTIONS=detect_leaks=1 ./_build_artifacts/ex_asan_bug leak
```

## 整理记录

- 工程与日常实验从 Windows `D:\develop_ai\deepseek_develop\知了的后端计划\验收记录\W3` 同步到本目录。
- `d6.test/1.cpp` → `task_runner.cpp`；`test1/1.cpp` → `daily/day5/asan_probe.cpp`；可执行文件移入 `_build_artifacts/`。
- `capture_demo.cpp` 与 `asan_leak_report.txt` 原缺失，本次补建 / 实测生成。
- 修复 `ex_task_runner.cpp`、`test_task_runner.cpp` 中 move-only 闭包无法存入 `std::function` 的编译错误（改用 `shared_ptr` 捕获并注释说明）。
- `W3讲清.m4a` 未找到，需重录 3–5 分钟。
