# C++ / Linux 系统编程

按周推进的 C++ 与 Linux 系统编程训练：每周一个主题，一个可验收的产出。

- 时间：2026-09-28 起，每周约 7 小时
- 目标：把 C++ 与 Linux 系统编程的地基打牢，积累能讲清楚的项目

## 进度

| 周 | 日期 | 主题 | 产出 | 目录 |
| :-- | :-- | :-- | :-- | :-- |
| W1–W2 | 09-28 ~ 10-11 | 工具链与环境、现代 C++ 基础 | 未留代码 | — |
| W3 | 10-12 ~ 10-18 | lambda / std::function / RAII / CMake / ASan | `TaskRunner`、`ScopeGuard`、ASan 泄漏报告 | `w3/` |
| W4 | 10-19 ~ 10-25 | 文件 I/O 与文件描述符 | `cat` 的文件读取版 | `w4/` |
| W5 | 10-26 ~ 11-01 | 进程：fork / exec / waitpid、僵尸与孤儿、地址空间 | `my_shell_exec` | `w5/`、`d6/` |
| W6 | 11-02 ~ 11-08 | IPC：管道、共享内存、信号 | 支持管道的简易 shell（进行中） | `w6/` |

## 目录

```
develop/
├── w3/      现代 C++ 与构建（含 CMake 工程与单元测试）
├── w4/      文件 I/O
├── w5/      进程
└── d6/      shell exec 雏形
```

## 构建

`w3/cmake_project` 是一个完整的 CMake 工程（核心库 + 示例 + 单元测试）：

```bash
cd w3/cmake_project
cmake --preset linux-debug
cmake --build --preset linux-debug
ctest --preset linux-debug
```

带 ASan/UBSan 的配置见 `cmake/Sanitizers.cmake` 与 `linux-asan` preset。

## 说明

这里的代码是学习过程中的练习，部分参考了教材与文档；每周的验收项会另外闭卷重写一遍。