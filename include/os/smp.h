#ifndef SMP_H
#define SMP_H
#include <type.h>
#include <os/lock.h>

#define MASK_ZERO 1
#define MASK_ONE 2
#define MASK_ALL 3

#define NR_CPUS 2
extern void smp_init();
extern void wakeup_other_hart();
extern uint64_t get_current_cpu_id();
extern void lock_kernel();
extern void unlock_kernel();
void wait_other_hart(int hart_id);
void other_hart_ready();

#endif /* SMP_H */
