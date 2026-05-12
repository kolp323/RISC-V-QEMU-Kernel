#include <asm.h>
#include <csr.h>

#define MODE_MASK 0xfffffffffffffffc  
#define DIRECT (0x0 & ~MODE_MASK)                 
#define Vectored (0x1 & ~MODE_MASK)

ENTRY(setup_exception)

  /* TODO: [p2-task3] save exception_handler_entry into STVEC */
  la      t0, exception_handler_entry
  andi    t0, t0, MODE_MASK // 去掉最后两位
  ori     t0, t0, DIRECT // 设置为DIRECT MODE
  csrw    stvec, t0

  # ret // p2-task3 end

  /* TODO: [p2-task4] enable interrupts globally */
  // 防止ra被覆盖
  addi    sp, sp, -8
  sd      ra, 0(sp)
  call    enable_interrupt // call会将本身下一条指令的地址写入ra
  ld      ra, 0(sp)
  addi    sp, sp, 8
  jr      ra
ENDPROC(setup_exception)
