# RISC-V-QEMU-Kernel

RISC-V-QEMU-Kernel is a portfolio-oriented RISC-V teaching kernel project running on QEMU. It integrates boot code, trap handling, process scheduling, synchronization primitives, virtual memory, system calls, file-system support, network-related drivers, a tiny C library and user-level test programs into a single cleaned kernel codebase.

本项目由操作系统实验代码重新整理而来，去掉了冗余的分支式课程提交结构，保留最终阶段中具有展示价值的源码，并将其包装为一个完整的 RISC-V / QEMU 教学内核项目。

## Project Highlights

- **RISC-V boot and trap flow**: includes boot block, startup code, trap entry, context switching support and linker script configuration.
- **Kernel process system**: provides PCB management, scheduler, process/task loading, sleep queues, CPU affinity and SMP-related support.
- **Synchronization primitives**: includes locks, barriers, condition variables and mailbox-style communication.
- **Virtual memory and paging**: implements page allocation, page table sharing/mapping, page fault handling, shared memory and swap-related modules.
- **System-call and user runtime support**: keeps syscall dispatch, tiny libc wrappers, shell utilities and user-space test programs.
- **Device and I/O support**: includes PLIC, screen, E1000/network-related code, pipe and file-system modules.
- **QEMU/GDB workflow**: keeps the Makefile, linker script and `.gdbinit` for low-level kernel debugging context.

## Repository Layout

```text
RISC-V-QEMU-Kernel/
├── arch/riscv/          # RISC-V boot, trap, startup, CSR and architecture support
├── drivers/             # PLIC, screen and E1000-related drivers
├── include/             # Kernel, syscall, type and OS subsystem headers
├── init/                # Kernel entry initialization
├── kernel/              # Core kernel subsystems
│   ├── fs/              # File-system support
│   ├── irq/             # Interrupt and page fault handling
│   ├── loader/          # Program/task loading
│   ├── locking/         # Locks, barriers, conditions and mailboxes
│   ├── mm/              # Physical/virtual memory, mapping, swap and shared memory
│   ├── net/             # Network and stream support
│   ├── pipe/            # Pipe implementation
│   ├── sched/           # Process table, scheduler, timing and workload logic
│   ├── smp/             # Multi-core support
│   ├── syscall/         # System-call dispatch
│   └── task/            # Task management
├── libs/                # Kernel-side support library
├── test/                # User programs and staged kernel tests
├── tiny_libc/           # User-side libc/syscall wrapper support
├── tools/               # Image/network helper tools and scripts
├── Makefile             # QEMU-oriented build entry from the original project
├── riscv.lds            # RISC-V linker script
└── docs/                # Portfolio documentation and architecture notes
```

## Kernel Architecture

### Boot and Trap Handling

Key files:

```text
arch/riscv/boot/bootblock.S
arch/riscv/kernel/head.S
arch/riscv/kernel/start.S
arch/riscv/kernel/entry.S
arch/riscv/kernel/trap.S
riscv.lds
```

These files define the low-level startup path, kernel entry, trap transition and memory layout used by the QEMU RISC-V environment.

### Scheduling and Process Management

Key files:

```text
kernel/sched/proc.c
kernel/sched/sched.c
kernel/sched/time.c
kernel/sched/workload.c
include/os/proc.h
include/os/sched.h
```

This subsystem manages process control blocks, ready/sleep queues, CPU affinity masks, context switching and timer-driven scheduling behavior.

### Memory Management

Key files:

```text
kernel/mm/mm.c
kernel/mm/free.c
kernel/mm/shm.c
kernel/mm/swap.c
kernel/irq/page_fault.c
include/os/mm.h
include/os/swap.h
```

The memory subsystem covers page allocation, physical/virtual address conversion, user page mapping, shared memory and swap-related support.

### Synchronization and IPC

Key files:

```text
kernel/locking/lock.c
kernel/locking/barrier.c
kernel/locking/condition.c
kernel/locking/mbox.c
kernel/pipe/pipe.c
```

The kernel includes lock primitives, barriers, condition variables, mailbox communication and pipe support for process coordination.

### System Calls and User Runtime

Key files:

```text
kernel/syscall/syscall.c
tiny_libc/syscall.c
tiny_libc/shell_command.c
test/shell.c
```

The public project preserves both the kernel-side syscall dispatcher and user-side wrappers/tests, making it easier to understand the complete syscall path.

## Tests and User Programs

The `test/` tree contains user programs grouped by subsystem milestones:

- `test_project1`: basic user programs and loading tests,
- `test_project2`: scheduling, sleep and locking tests,
- `test_project3`: synchronization, threads and multicore tests,
- `test_project4`: memory, swap, pipe and IPC tests,
- `test_project5`: network send/receive tests,
- `test_project6`: file-system-oriented tests.

The branch-per-project history was not kept in the public packaging. Instead, these tests remain as a unified test suite for the final integrated kernel.

## Build Notes

This repository preserves the original QEMU-oriented build files, but it is primarily packaged for source review and portfolio display. The expected environment may include:

- RISC-V cross compiler,
- QEMU RISC-V system emulator,
- GDB for low-level debugging,
- Make-based build flow.

Start from:

```text
Makefile
.gdbinit
riscv.lds
arch/riscv/
```

Generated images and local binary artifacts are intentionally excluded from the public repository.

## What Was Repackaged

This public repository was derived from a private branch-per-project operating-system lab repository. The public version:

- uses the final integrated kernel snapshot as the default codebase,
- removes redundant course branch structure,
- removes generated images, editor cache and binary artifacts,
- keeps source files that demonstrate kernel architecture and implementation depth,
- presents the project as one RISC-V / QEMU kernel system.

## Project Value

This project demonstrates:

- RISC-V low-level boot and trap handling,
- QEMU-based kernel development,
- process scheduling and context switching,
- synchronization and IPC design,
- virtual memory and paging implementation,
- syscall and tiny libc integration,
- file-system, pipe and network-related kernel modules,
- low-level debugging workflow with GDB/QEMU.
