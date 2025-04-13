/*
 * Copyright (C) 2025 pdnguyen of HCMC University of Technology VNU-HCM
 */

/* Sierra release
 * Source Code License Grant: The authors hereby grant to Licensee
 * personal permission to use and modify the Licensed Source Code
 * for the sole purpose of studying while attending the course CO2018.
 */

 #include "syscall.h"
 #include "common.h"
 
 #define __SYSCALL(nr, sym) extern int __##sym(struct pcb_t*,struct sc_regs*);
 #include "syscalltbl.lst"
 #undef  __SYSCALL
 
 /*
  * The sys_call_table[] is used for system calls, but to know the system
  * call address.
  */
 #define __SYSCALL(nr, sym) #nr "-" #sym,
 const char* sys_call_table[] = {
 #include "syscalltbl.lst"
 };
 #undef  __SYSCALL
 const int syscall_table_size = sizeof(sys_call_table)/sizeof(char*);
 
 int __sys_ni_syscall(struct pcb_t *caller, struct sc_regs *regs)
 {
    /*
     * DUMMY systemcall
     */
    printf("ERROR: Called non-implemented system call (nr=%d)\n", regs->orig_ax);
    printf("Available system calls:\n");
    
    for(int i = 0; i < syscall_table_size; i++)
       printf("%s\n", sys_call_table[i]);
    
    return 0;
 }
 
 #define __SYSCALL(nr, sym) case nr: return __##sym(caller,regs);
 int syscall(struct pcb_t *caller, uint32_t nr, struct sc_regs* regs)
 {
    regs->orig_ax = nr;
    
    switch (nr) {
    #include "syscalltbl.lst"
    default: return __sys_ni_syscall(caller, regs);
    }
    return 0;
 };