# RISC-V QEMU Kernel

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](https://opensource.org/licenses/MIT)

[**中文文档**](README_zh.md) | [**English**](README.md)

`RISC-V-QEMU-Kernel` is a lightweight, educational operating system kernel designed for the RISC-V architecture, running on the QEMU emulator. It integrates core OS components into a clean, standalone codebase, providing an excellent reference for learning low-level system programming, kernel architecture, and RISC-V instruction set behavior.

This project was originally derived from a comprehensive operating system laboratory curriculum. It has been refactored and consolidated into a unified kernel project, removing the staggered educational branch structure to present a complete, observable system.

## 🌟 Key Features

*   **Boot & Trap Architecture:** Complete RISC-V low-level startup path, including boot blocks, assembly initialization, trap entry/exit, context switching, and linker scripts.
*   **Process & Scheduling:** Robust process management featuring PCBs, a ready/sleep queue scheduler, process/task loading, CPU affinity, and SMP (Symmetric Multiprocessing) support.
*   **Synchronization Primitives:** Implementation of essential concurrency controls, including spinlocks, barriers, condition variables, and mailbox-style IPC.
*   **Memory Management:** Full virtual memory subsystem supporting physical page allocation, multi-level page table mapping, page fault handling, shared memory, and a functional swap mechanism.
*   **System Calls & Runtime:** A defined system call ABI with a kernel-side dispatcher, accompanied by a minimal `libc` (tiny libc) and interactive user-space shell tests.
*   **Device Drivers:** Support for essential QEMU virt machine peripherals, including the PLIC (Platform-Level Interrupt Controller), screen output, and E1000 network interfaces.
*   **File System & IPC:** Implementation of basic file system structures and Unix-style pipes.
*   **Debugging Ready:** Pre-configured with `Makefile`, linker scripts (`riscv.lds`), and `.gdbinit` for an immediate, low-level GDB debugging experience.

## 📂 Project Structure

```text
RISC-V-QEMU-Kernel/
├── arch/riscv/          # Architecture-specific code: boot, trap, startup, CSRs
├── drivers/             # Device drivers: PLIC, screen, E1000
├── include/             # Global headers for kernel subsystems and syscalls
├── init/                # Kernel entry point and initialization sequence
├── kernel/              # Core kernel modules
│   ├── fs/              # File system implementation
│   ├── irq/             # Interrupt and page fault handlers
│   ├── loader/          # ELF/binary loading mechanisms
│   ├── locking/         # Synchronization primitives
│   ├── mm/              # Memory management (PMM, VMM, Swap, SHM)
│   ├── net/             # Network stack basics
│   ├── pipe/            # Pipe IPC
│   ├── sched/           # Process scheduling and timer management
│   ├── smp/             # Multicore initialization and coordination
│   ├── syscall/         # System call routing
│   └── task/            # Task state management
├── libs/                # Kernel-space utility libraries (string, print, etc.)
├── test/                # User-space test applications and shell
├── tiny_libc/           # Minimal standard library for user-space programs
├── tools/               # Build helpers and network configuration scripts
├── Makefile             # Primary build script
├── riscv.lds            # Linker script for memory layout
└── docs/                # Architectural documentation and notes
```

## 🚀 Getting Started

### Prerequisites

To build and run this kernel, you need a standard RISC-V cross-compilation toolchain and the QEMU emulator installed on your system.

*   **RISC-V GNU Compiler Toolchain** (e.g., `riscv64-unknown-elf-gcc`)
*   **QEMU for RISC-V** (e.g., `qemu-system-riscv64`)
*   **GDB Multiarch** or RISC-V GDB (for debugging)
*   **Make**

### Building and Running

*(Note: Specific make targets depend on the internal `Makefile` structure. The following are typical examples.)*

1.  **Clone the repository:**
    ```bash
    git clone https://github.com/kolp323/RISC-V-QEMU-Kernel.git
    cd RISC-V-QEMU-Kernel
    ```

2.  **Build the kernel image:**
    ```bash
    make all
    ```

3.  **Run in QEMU:**
    ```bash
    make run
    ```

4.  **Debug with GDB:**
    Open two terminals.
    In terminal 1 (start QEMU waiting for GDB):
    ```bash
    make debug
    ```
    In terminal 2 (connect GDB):
    ```bash
    riscv64-unknown-elf-gdb -x .gdbinit
    ```

*(Please inspect the `Makefile` for exact targets if the above standard commands differ.)*

## 🛠️ Testing

The `test/` directory contains a suite of user-space programs designed to validate different subsystems of the kernel. These are logically grouped (e.g., `test_project1` through `test_project6`) representing the evolution of the kernel from basic execution to complex memory and network operations.

These tests are integrated into the final build to verify the holistic functionality of the OS.

## 🤝 Contributing

Contributions, issues, and feature requests are welcome! Feel free to check the [issues page](https://github.com/kolp323/RISC-V-QEMU-Kernel/issues).

If you are using this project for learning, bug fixes to existing modules or improvements to documentation are highly appreciated.

1. Fork the Project
2. Create your Feature Branch (`git checkout -b feature/AmazingFeature`)
3. Commit your Changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the Branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.