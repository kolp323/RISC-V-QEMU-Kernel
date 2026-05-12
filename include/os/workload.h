#ifndef INCLUDE_WORKLOAD_H_
#define INCLUDE_WORKLOAD_H_
#include <os/sched.h>

#define LENGTH 60
#define FLY_NUM 5		// 飞机数为5
extern const int check_point[FLY_NUM];
extern const int end_point[FLY_NUM];
extern const int check_to_end[FLY_NUM];

extern int stage;
extern int done_cnt;
typedef enum {
    CHECKPOINT,
    END
}stage_t;

void init_fly();
void reset_target_wakeup(const int* stage);
void do_workload_scheduler();
#endif