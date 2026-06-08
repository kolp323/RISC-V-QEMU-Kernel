# RISC-V QEMU 教学内核

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](https://opensource.org/licenses/MIT)

[**中文文档**](README_zh.md) | [**English**](README.md)

`RISC-V-QEMU-Kernel` 是一个为 RISC-V 架构设计的轻量级教学操作系统内核，运行在 QEMU 模拟器上。本项目将操作系统的核心组件整合到一个干净、独立的源码库中，为学习底层系统编程、内核架构和 RISC-V 指令集行为提供了绝佳的参考。

本项目最初由一套完整的操作系统实验课程代码衍生而来。经过重构与整合，去除了原有的阶梯式教学分支结构，呈现为一个完整、可观测的统一内核项目。

## 🌟 核心特性

*   **启动与异常架构:** 完整的 RISC-V 底层启动路径，包括引导块 (boot block)、汇编初始化、异常入口/出口、上下文切换以及链接器脚本。
*   **进程与调度:** 健壮的进程管理，包含 PCB 管理、就绪/睡眠队列调度器、进程/任务加载、CPU 亲和性以及 SMP (对称多处理) 支持。
*   **同步原语:** 实现了关键的并发控制机制，包括自旋锁、屏障 (barrier)、条件变量以及邮箱式进程间通信 (IPC)。
*   **内存管理:** 完整的虚拟内存子系统，支持物理页分配、多级页表映射、缺页异常处理、共享内存以及功能性的 Swap 交换机制。
*   **系统调用与运行时:** 定义了系统调用 ABI 和内核侧分发器，并附带一个极简的 `libc` (tiny libc) 和交互式的用户态 Shell 测试程序。
*   **设备驱动:** 支持 QEMU virt 机器的基本外设，包括 PLIC (平台级中断控制器)、屏幕输出和 E1000 网络接口。
*   **文件系统与 IPC:** 实现了基础的文件系统结构和类 Unix 管道 (Pipe)。
*   **开箱即用的调试:** 预配置了 `Makefile`、链接脚本 (`riscv.lds`) 和 `.gdbinit`，可立即体验底层的 GDB 调试流程。

## 📂 项目结构

```text
RISC-V-QEMU-Kernel/
├── arch/riscv/          # 架构相关代码：启动、异常、CSR寄存器
├── drivers/             # 设备驱动：PLIC、屏幕、E1000
├── include/             # 内核子系统和系统调用的全局头文件
├── init/                # 内核入口点和初始化序列
├── kernel/              # 核心内核模块
│   ├── fs/              # 文件系统实现
│   ├── irq/             # 中断和缺页异常处理
│   ├── loader/          # ELF/二进制程序加载机制
│   ├── locking/         # 同步原语 (锁、条件变量等)
│   ├── mm/              # 内存管理 (物理/虚拟内存、Swap、共享内存)
│   ├── net/             # 基础网络栈
│   ├── pipe/            # 管道 IPC
│   ├── sched/           # 进程调度和定时器管理
│   ├── smp/             # 多核初始化与协同
│   ├── syscall/         # 系统调用路由
│   └── task/            # 任务状态管理
├── libs/                # 内核空间工具库 (字符串、打印等)
├── test/                # 用户态测试程序和 Shell
├── tiny_libc/           # 用于用户态程序的极简标准库
├── tools/               # 构建辅助工具和网络配置脚本
├── Makefile             # 主构建脚本
├── riscv.lds            # 控制内存布局的链接脚本
└── docs/                # 架构文档和笔记
```

## 🚀 快速开始

### 环境依赖

要构建并运行此内核，您的系统需要安装标准的 RISC-V 交叉编译工具链和 QEMU 模拟器。

*   **RISC-V GNU Compiler Toolchain** (例如: `riscv64-unknown-elf-gcc`)
*   **QEMU for RISC-V** (例如: `qemu-system-riscv64`)
*   **GDB Multiarch** 或 RISC-V GDB (用于调试)
*   **Make**

### 构建与运行

*(注意：具体的 Make 目标取决于内部 `Makefile` 的定义。以下为典型示例。)*

1.  **克隆仓库:**
    ```bash
    git clone https://github.com/kolp323/RISC-V-QEMU-Kernel.git
    cd RISC-V-QEMU-Kernel
    ```

2.  **构建内核镜像:**
    ```bash
    make all
    ```

3.  **在 QEMU 中运行:**
    ```bash
    make run
    ```

4.  **使用 GDB 调试:**
    打开两个终端。
    在终端 1 (启动 QEMU 并等待 GDB 连接):
    ```bash
    make debug
    ```
    在终端 2 (连接 GDB):
    ```bash
    riscv64-unknown-elf-gdb -x .gdbinit
    ```

*(如果实际命令与上述标准命令有所不同，请查阅项目根目录下的 `Makefile` 获取准确指令。)*

## 🛠️ 测试

`test/` 目录包含了一系列用户态程序，用于验证内核的各个子系统。这些程序在逻辑上进行了分组（例如从 `test_project1` 到 `test_project6`），代表了内核从基础执行到复杂的内存和网络操作的演进过程。

这些测试已被集成到最终的构建中，以验证整个操作系统的综合功能。

## 🤝 参与贡献

欢迎任何形式的贡献、提交 Issue 或功能请求！请查看 [Issues 页面](https://github.com/kolp323/RISC-V-QEMU-Kernel/issues)。

如果您正在使用本项目进行学习，我们非常欢迎您提交针对现有模块的 Bug 修复或对文档的改进。

1. Fork 本项目
2. 创建您的特性分支 (`git checkout -b feature/AmazingFeature`)
3. 提交您的更改 (`git commit -m 'Add some AmazingFeature'`)
4. 推送到该分支 (`git push origin feature/AmazingFeature`)
5. 提交一个 Pull Request (PR)

## 📄 开源协议

本项目采用 MIT 开源协议 - 详情请参阅 [LICENSE](LICENSE) 文件。