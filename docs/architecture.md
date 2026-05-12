# Architecture Notes

## System Story

`RISC-V-QEMU-Kernel` is packaged as a single integrated teaching kernel instead of six separate course branches.

The recommended reading path is:

1. `arch/riscv/` - boot, trap and architecture-specific runtime.
2. `init/main.c` - kernel initialization entry.
3. `kernel/sched/` - process table and scheduler.
4. `kernel/mm/` and `kernel/irq/page_fault.c` - memory management and page faults.
5. `kernel/syscall/` and `tiny_libc/` - syscall path from user program to kernel.
6. `test/` - user programs that exercise each subsystem.

## Subsystem Map

| Subsystem | Main Paths | Purpose |
| --- | --- | --- |
| Boot / traps | `arch/riscv/boot`, `arch/riscv/kernel`, `riscv.lds` | Start the kernel, enter supervisor code and handle traps |
| Scheduler | `kernel/sched`, `include/os/proc.h`, `include/os/sched.h` | Manage PCB state, ready/sleep queues, CPU affinity and switching |
| Memory | `kernel/mm`, `include/os/mm.h`, `kernel/irq/page_fault.c` | Allocate pages, map user memory, handle page faults and swap/shared memory |
| Synchronization | `kernel/locking` | Locks, barriers, condition variables and mailboxes |
| IPC / FS / pipe | `kernel/pipe`, `kernel/fs` | Pipe and file-system support |
| Syscalls | `kernel/syscall`, `include/sys`, `tiny_libc/syscall.c` | Kernel/user boundary and user-side wrappers |
| Drivers | `drivers` | PLIC, screen and E1000-related support |
| User programs | `test`, `tiny_libc` | Shell, tests and user runtime support |

## Milestone Narrative

The original private repository used branch-per-project organization. This public version keeps the final integrated source tree and represents the milestone progression in documentation:

- Project 1: boot, loader and basic user programs,
- Project 2: process scheduling, timer and locking basics,
- Project 3: synchronization, threads and multicore behavior,
- Project 4: virtual memory, swap, shared memory and IPC,
- Project 5: network-oriented tests and E1000 support,
- Project 6: file-system-oriented integration and final cleanup.

## Public Packaging Decisions

Excluded content:

- generated `image` files,
- local editor cache,
- compiled helper binaries,
- backup files,
- redundant branch history.

Preserved content:

- final integrated kernel source,
- RISC-V architecture code,
- kernel subsystems,
- user-space tests,
- tiny libc and syscall wrappers,
- source-level tools and debug configuration.
