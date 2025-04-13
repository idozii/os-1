/*
 * Copyright (C) 2025 pdnguyen of HCMC University of Technology VNU-HCM
 */

/* Sierra release
 * Source Code License Grant: The authors hereby grant to Licensee
 * personal permission to use and modify the Licensed Source Code
 * for the sole purpose of studying while attending the course CO2018.
 */

#include "common.h"
#include "syscall.h"
#include "stdio.h"
#include "libmem.h"
#include "string.h"
#include "queue.h"
#include "stdlib.h"
 
// Remove mutex for now to simplify debugging
// #include <pthread.h>
// extern pthread_mutex_t queue_lock;

int __sys_killall(struct pcb_t *caller, struct sc_regs* regs)
{
    char proc_name[100];
    uint32_t data;

    // Hardcode for demo only
    uint32_t memrg = regs->a1;
    
    /* Get name of the target proc */
    int i = 0;
    data = 0;
    while(data != -1) {
        libread(caller, memrg, i, &data);
        proc_name[i] = data;
        if(data == -1) proc_name[i] = '\0';
        i++;
    }
    printf("The procname retrieved from memregionid %d is \"%s\"\n", memrg, proc_name);

    /* Traverse proclist to terminate the proc */
    int terminated_count = 0;
    struct pcb_t *proc = NULL;
    
    // Remove mutex locking for now
    // pthread_mutex_lock(&queue_lock);
    
    // Safely check if running_list exists
    if (caller->running_list != NULL) {
        struct queue_t *running = caller->running_list;
        
        // Safety check the queue size
        if (running->size > 0) {
            // We need to be careful when removing elements from the queue during iteration
            // Start from the end to avoid index shifting issues
            for (i = running->size - 1; i >= 0; i--) {
                proc = running->proc[i];
                if (proc != NULL && strcmp(proc->path, proc_name) == 0) {
                    printf("Terminating process %s (pid=%d) from running list\n", 
                        proc->path, proc->pid);
                    
                    // Remove the process from the queue
                    // Shift remaining elements 
                    int j;
                    for (j = i; j < running->size - 1; j++) {
                        running->proc[j] = running->proc[j + 1];
                    }
                    running->size--;
                    
                    // Free process resources
                    // This depends on how your OS manages resources
                    // For example:
                    free(proc);
                    
                    terminated_count++;
                }
            }
        }
    }
    
    // Traverse ready queues with safety checks
    #ifdef MLQ_SCHED
    // MLQ scheduler has multiple priority queues
    if (caller->mlq_ready_queue != NULL) {
        for (int prio = 0; prio < MAX_PRIO; prio++) {
            struct queue_t *ready_q = &(caller->mlq_ready_queue[prio]);
            if (ready_q != NULL && ready_q->size > 0) {
                // Iterate backwards for safe removal
                for (i = ready_q->size - 1; i >= 0; i--) {
                    proc = ready_q->proc[i];
                    if (proc != NULL && strcmp(proc->path, proc_name) == 0) {
                        printf("Terminating process %s (pid=%d) from ready queue (prio=%d)\n", 
                            proc->path, proc->pid, prio);
                        
                        // Remove the process from the queue
                        int j;
                        for (j = i; j < ready_q->size - 1; j++) {
                            ready_q->proc[j] = ready_q->proc[j + 1];
                        }
                        ready_q->size--;
                        
                        // Free process resources
                        free(proc);
                        
                        terminated_count++;
                    }
                }
            }
        }
    }
    #else
    // Single ready queue
    if (caller->ready_queue != NULL) {
        struct queue_t *ready_q = caller->ready_queue;
        if (ready_q->size > 0) {
            // Iterate backwards for safe removal
            for (i = ready_q->size - 1; i >= 0; i--) {
                proc = ready_q->proc[i];
                if (proc != NULL && strcmp(proc->path, proc_name) == 0) {
                    printf("Terminating process %s (pid=%d) from ready queue\n", 
                        proc->path, proc->pid);
                    
                    // Remove the process from the queue
                    int j;
                    for (j = i; j < ready_q->size - 1; j++) {
                        ready_q->proc[j] = ready_q->proc[j + 1];
                    }
                    ready_q->size--;
                    
                    // Free process resources
                    free(proc);
                    
                    terminated_count++;
                }
            }
        }
    }
    #endif
    
    // Remove mutex unlocking for now
    // pthread_mutex_unlock(&queue_lock);
    
    printf("Total of %d processes named '%s' terminated\n", terminated_count, proc_name);
    return terminated_count;
}
