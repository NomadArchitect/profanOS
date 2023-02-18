#ifndef TASK_H
#define TASK_H

#include <type.h>

#define PROCESS_RUNNING  0
#define PROCESS_WAITING  1
#define PROCESS_SLEEPING 2
#define PROCESS_KILLED   3
#define PROCESS_DEAD     4


typedef struct {
    uint32_t eax, ebx, ecx, edx, esi, edi, esp, ebp, eip, eflags, cr3;
} proc_rgs_t;

typedef struct {
    proc_rgs_t regs;
    int pid, ppid, priority;
    int state, sleep_to;
    uint32_t esp_addr;
    uint8_t *run_mem;
    void *custom;
    char name[64];
} process_t;

int process_init();

// switch.asm
extern void process_asm_switch(proc_rgs_t *old, proc_rgs_t *new);

#endif
