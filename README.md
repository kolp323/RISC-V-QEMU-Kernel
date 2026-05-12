# RISC-V-QEMU-Kernel

RISC-V-QEMU-Kernel is a small teaching operating-system kernel for the RISC-V `virt` machine in QEMU. The tree contains the kernel boot path, trap handling, scheduler, synchronization primitives, memory management, system calls, device drivers, a tiny user runtime, and user-space test programs.

## Features

- **RISC-V boot flow**: boot block, startup assembly, linker script, CSR helpers, and QEMU entry configuration.
- **Trap and interrupt handling**: trap entry code, syscall dispatch, timer handling, external interrupt support, and page-fault handling.
- **Process management**: process control blocks, task loading, ready/sleep queues, wait/exit handling, CPU affinity, and SMP support.
- **Synchronization and IPC**: mutex locks, barriers, condition variables, mailboxes, and pipes.
- **Memory management**: physical page allocation, virtual mapping, user page tables, shared memory, and swap-related code.
- **File-system and I/O support**: file-system routines, screen output, PLIC support, and E1000/network modules.
- **User-space runtime and tests**: tiny libc, syscall wrappers, shell code, and staged user programs under `test/`.

## Repository Layout

```text
RISC-V-QEMU-Kernel/
|-- arch/riscv/          # RISC-V boot, trap, startup, CSR, and architecture support
|-- drivers/             # PLIC, screen, and E1000-related drivers
|-- include/             # Kernel, syscall, type, and subsystem headers
|-- init/                # Kernel initialization entry
|-- kernel/              # Core kernel subsystems
|   |-- fs/              # File-system support
|   |-- irq/             # Interrupt and page-fault handling
|   |-- loader/          # Program and task loading
|   |-- locking/         # Locks, barriers, conditions, and mailboxes
|   |-- mm/              # Physical/virtual memory, mappings, swap, and shared memory
|   |-- net/             # Network and stream support
|   |-- pipe/            # Pipe implementation
|   |-- sched/           # Process table, scheduler, timing, and workload logic
|   |-- smp/             # Multi-core support
|   |-- syscall/         # System-call dispatch
|   `-- task/            # Task management
|-- libs/                # Kernel-side support library
|-- test/                # Shell and user-space test programs
|-- tiny_libc/           # User-side libc and syscall wrappers
|-- tools/               # Host-side image creation helper
|-- Makefile             # Build, run, and debug targets
|-- riscv.lds            # RISC-V linker script
`-- .gdbinit             # GDB helper configuration
```

## Build Environment

The Makefile expects a Linux-style RISC-V development environment. The default tool and path settings are defined near the top of `Makefile`:

```make
CROSS_PREFIX = riscv64-unknown-linux-gnu-
DIR_OSLAB    = $(HOME)/OSLab-RISC-V
DIR_QEMU     = $(DIR_OSLAB)/qemu
DIR_UBOOT    = $(DIR_OSLAB)/u-boot
```

Required components include:

- `riscv64-unknown-linux-gnu-gcc`, `ar`, `objdump`, and `gdb`
- QEMU RISC-V system emulator built under `$(DIR_QEMU)`
- U-Boot image at `$(DIR_UBOOT)/u-boot`
- standard Linux build tools such as `make` and `gcc`

If your tools are installed elsewhere, update the variables in `Makefile` before building.

## Build and Run

Build the boot block, kernel, tiny libc, user programs, and image:

```sh
make
```

Remove generated build artifacts:

```sh
make clean
```

Run the kernel in QEMU:

```sh
make run
```

Run with two QEMU CPUs:

```sh
make run-smp
```

Start QEMU and wait for GDB on port `1234`:

```sh
make debug
```

Attach GDB to a waiting debug session:

```sh
make gdb
```

Network targets are also available, but they expect a Linux TAP setup matching the QEMU scripts configured by `DIR_QEMU`:

```sh
make run-net
make debug-net
```

## User Programs and Tests

The `PROJECT_IDX` variable in `Makefile` selects which staged test directory is built into the image:

```make
PROJECT_IDX = 6
DIR_TEST_PROJ = $(DIR_TEST)/test_project$(PROJECT_IDX)
```

The test directories cover different kernel subsystems:

- `test_project1`: basic user programs and loading tests
- `test_project2`: scheduling, sleep, timer, and locking tests
- `test_project3`: synchronization, threads, mailbox, affinity, and multicore tests
- `test_project4`: memory, swap, pipe, and IPC tests
- `test_project5`: network send/receive tests
- `test_project6`: file-system-oriented tests

To build a different test group, change `PROJECT_IDX` and rebuild.

## Kernel Subsystems

### Boot and Trap Handling

Relevant files:

```text
arch/riscv/boot/bootblock.S
arch/riscv/kernel/head.S
arch/riscv/kernel/start.S
arch/riscv/kernel/entry.S
arch/riscv/kernel/trap.S
riscv.lds
```

These files define the low-level startup path, kernel entry, trap transition, and memory layout used by the QEMU RISC-V target.

### Scheduling and Process Management

Relevant files:

```text
kernel/sched/proc.c
kernel/sched/sched.c
kernel/sched/time.c
kernel/sched/workload.c
include/os/proc.h
include/os/sched.h
```

This subsystem manages process control blocks, ready and sleep queues, CPU affinity masks, context switching, and timer-driven scheduling.

### Memory Management

Relevant files:

```text
kernel/mm/mm.c
kernel/mm/free.c
kernel/mm/shm.c
kernel/mm/swap.c
kernel/irq/page_fault.c
include/os/mm.h
include/os/swap.h
```

The memory subsystem covers page allocation, physical/virtual address conversion, user page mapping, shared memory, page-fault handling, and swap-related support.

### Synchronization and IPC

Relevant files:

```text
kernel/locking/lock.c
kernel/locking/barrier.c
kernel/locking/condition.c
kernel/locking/mbox.c
kernel/pipe/pipe.c
```

The kernel includes lock primitives, barriers, condition variables, mailbox communication, and pipe support for process coordination.

### System Calls and User Runtime

Relevant files:

```text
kernel/syscall/syscall.c
arch/riscv/include/asm/unistd.h
include/sys/syscall.h
tiny_libc/syscall.c
tiny_libc/shell_command.c
test/shell.c
```

These files connect user programs to kernel services through syscall numbers, user-side wrappers, the syscall dispatcher, and shell commands.

## Generated Files

The build creates local artifacts such as `build/`, image files, ELF files, and disassembly output. These files are generated from source and are not required in version control.
