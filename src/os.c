
#include "cpu.h"
#include "timer.h"
#include "sched.h"
#include "loader.h"
#include "mm.h"

#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static int time_slot;
static int num_cpus;
static int done = 0;

#ifdef MM_PAGING
static int memramsz;
static int memswpsz[PAGING_MAX_MMSWP];

struct mmpaging_ld_args {
	/* A dispatched argument struct to compact many-fields passing to loader */
	int vmemsz;
	struct memphy_struct *mram;
	struct memphy_struct **mswp;
	struct memphy_struct *active_mswp;
	int active_mswp_id;
	struct timer_id_t  *timer_id;
};
#endif

static struct ld_args{
	char ** path;
	unsigned long * start_time;
#ifdef MLQ_SCHED
	unsigned long * prio;
#endif
} ld_processes;
int num_processes;

struct cpu_args {
	struct timer_id_t * timer_id;
	int id;
};


static void * cpu_routine(void * args) {
	struct timer_id_t * timer_id = ((struct cpu_args*)args)->timer_id;
	int id = ((struct cpu_args*)args)->id;
	/* Check for new process in ready queue */
	int time_left = 0;
	struct pcb_t * proc = NULL;
	while (1) {
		/* Check the status of current process */
		if (proc == NULL) {
			/* No process is running, the we load new process from
		 	* ready queue */
			proc = get_proc();
			if (proc == NULL) {
                           next_slot(timer_id);
                           continue; /* First load failed. skip dummy load */
                        }
		}else if (proc->pc == proc->code->size) {
			/* The porcess has finish it job */
			printf("\tCPU %d: Processed %2d has finished\n",
				id ,proc->pid);
			free(proc);
			proc = get_proc();
			time_left = 0;
		}else if (time_left == 0) {
			/* The process has done its job in current time slot */
			printf("\tCPU %d: Put process %2d to run queue\n",
				id, proc->pid);
			put_proc(proc);
			proc = get_proc();
		}
		
		/* Recheck process status after loading new process */
		if (proc == NULL && done) {
			/* No process to run, exit */
			printf("\tCPU %d stopped\n", id);
			break;
		}else if (proc == NULL) {
			/* There may be new processes to run in
			 * next time slots, just skip current slot */
			next_slot(timer_id);
			continue;
		}else if (time_left == 0) {
			printf("\tCPU %d: Dispatched process %2d\n",
				id, proc->pid);
			time_left = time_slot;
		}
		
		/* Run current process */
		run(proc);
		time_left--;
		next_slot(timer_id);
	}
	detach_event(timer_id);
	pthread_exit(NULL);
}

static void * ld_routine(void * args) {
#ifdef MM_PAGING
    struct memphy_struct* mram = ((struct mmpaging_ld_args *)args)->mram;
    struct memphy_struct** mswp = ((struct mmpaging_ld_args *)args)->mswp;
    struct memphy_struct* active_mswp = ((struct mmpaging_ld_args *)args)->active_mswp;
    struct timer_id_t * timer_id = ((struct mmpaging_ld_args *)args)->timer_id;
#else
    struct timer_id_t * timer_id = (struct timer_id_t*)args;
#endif
    int i = 0;
    printf("ld_routine\n");
    while (i < num_processes) {
        struct pcb_t * proc = load(ld_processes.path[i]);
#ifdef MLQ_SCHED
        proc->prio = ld_processes.prio[i];
#endif
        while (current_time() < ld_processes.start_time[i]) {
            next_slot(timer_id);
        }
#ifdef MM_PAGING
        proc->mm = malloc(sizeof(struct mm_struct));
        init_mm(proc->mm, proc);
        proc->mram = mram;
        proc->mswp = mswp;
        proc->active_mswp = active_mswp;
#endif
        printf("\tLoaded a process at %s, PID: %d%s%lu\n",
            ld_processes.path[i], proc->pid, 
#ifdef MLQ_SCHED
            " PRIO: ", ld_processes.prio[i]
#else
            "", 0
#endif
        );
        add_proc(proc);
        free(ld_processes.path[i]);
        i++;
        next_slot(timer_id);
    }
    free(ld_processes.path);
    free(ld_processes.start_time);
#ifdef MLQ_SCHED
    free(ld_processes.prio);
#endif
    done = 1;
    detach_event(timer_id);
    pthread_exit(NULL);
}

static void read_config(const char * path) {
    FILE * file;
    if ((file = fopen(path, "r")) == NULL) {
        printf("Cannot find configure file at %s\n", path);
        exit(1);
    }
    
    if (fscanf(file, "%d %d %d", &time_slot, &num_cpus, &num_processes) != 3) {
        printf("Error reading configuration parameters\n");
        exit(1);
    }
    
    int c;
    while ((c = fgetc(file)) != EOF && c != '\n');
    
    ld_processes.path = (char**)malloc(sizeof(char*) * num_processes);
    ld_processes.start_time = (unsigned long*)
        malloc(sizeof(unsigned long) * num_processes);
        
#ifdef MM_PAGING
    int sit;
#ifdef MM_FIXED_MEMSZ
    /* Use fixed memory sizes for backward compatibility */
    memramsz    =  0x100000;
    memswpsz[0] = 0x1000000;
    for(sit = 1; sit < PAGING_MAX_MMSWP; sit++)
        memswpsz[sit] = 0;
#else
    /* Read memory configuration if present */
    char peek_line[256];
    long file_pos = ftell(file);
    
    if (fgets(peek_line, sizeof(peek_line), file) == NULL) {
        printf("Error reading next line\n");
        exit(1);
    }
    
    /* Check if this line has memory config (no letters) */
    int is_mem_config = 1;
    for (char *p = peek_line; *p; p++) {
        if ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z')) {
            is_mem_config = 0;
            break;
        }
    }
    
    if (is_mem_config) {
        /* Parse memory configuration */
        if (sscanf(peek_line, "%d", &memramsz) != 1) {
            printf("Error parsing RAM size\n");
            exit(1);
        }
        
        char *pos = peek_line;
        while (*pos && *pos != ' ') pos++; // Skip first number
        
        for (sit = 0; sit < PAGING_MAX_MMSWP; sit++) {
            while (*pos && *pos == ' ') pos++; // Skip spaces
            if (*pos) {
                memswpsz[sit] = atoi(pos);
                while (*pos && *pos != ' ') pos++; // Skip number
            } else {
                memswpsz[sit] = 0;
            }
        }
    } else {
        /* No memory config, use fixed sizes */
        fseek(file, file_pos, SEEK_SET); // Go back to start of line
        memramsz    =  0x100000;
        memswpsz[0] = 0x1000000;
        for(sit = 1; sit < PAGING_MAX_MMSWP; sit++)
            memswpsz[sit] = 0;
    }
#endif
#endif

#ifdef MLQ_SCHED
    ld_processes.prio = (unsigned long*)
        malloc(sizeof(unsigned long) * num_processes);
#endif

    int i;
    for (i = 0; i < num_processes; i++) {
        ld_processes.path[i] = (char*)malloc(sizeof(char) * 100);
        ld_processes.path[i][0] = '\0';
        strcat(ld_processes.path[i], "input/proc/");
        
        char proc[100];
        char line[256];
        
        if (fgets(line, sizeof(line), file) == NULL) {
            printf("Error reading process %d information\n", i);
            exit(1);
        }
        
        // Remove newline if present
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') {
            line[len-1] = '\0';
        }
        
#ifdef MLQ_SCHED
        if (sscanf(line, "%lu %s %lu", 
                &ld_processes.start_time[i], proc, &ld_processes.prio[i]) != 3) {
            printf("Error parsing process %d info\n", i);
            exit(1);
        }
#else
        if (sscanf(line, "%lu %s", &ld_processes.start_time[i], proc) != 2) {
            printf("Error parsing process %d info\n", i);
            exit(1);
        }
#endif
        strcat(ld_processes.path[i], proc);
        
        // Debug output
        printf("Process %d: start=%lu, path=%s", 
            i, ld_processes.start_time[i], ld_processes.path[i]);
#ifdef MLQ_SCHED
        printf(", priority=%lu", ld_processes.prio[i]);
#endif
        printf("\n");
    }
    
    fclose(file);
}

int main(int argc, char * argv[]) {
	/* Read config */
	if (argc != 2) {
		printf("Usage: os [path to configure file]\n");
		return 1;
	}
	char path[100];
	path[0] = '\0';
	strcat(path, "input/");
	strcat(path, argv[1]);
	read_config(path);

	pthread_t * cpu = (pthread_t*)malloc(num_cpus * sizeof(pthread_t));
	struct cpu_args * args =
		(struct cpu_args*)malloc(sizeof(struct cpu_args) * num_cpus);
	pthread_t ld;
	
	/* Init timer */
	int i;
	for (i = 0; i < num_cpus; i++) {
		args[i].timer_id = attach_event();
		args[i].id = i;
	}
	struct timer_id_t * ld_event = attach_event();
	start_timer();

#ifdef MM_PAGING
	/* Init all MEMPHY include 1 MEMRAM and n of MEMSWP */
	int rdmflag = 1; /* By default memphy is RANDOM ACCESS MEMORY */

	struct memphy_struct mram;
	struct memphy_struct mswp[PAGING_MAX_MMSWP];

	/* Create MEM RAM */
	init_memphy(&mram, memramsz, rdmflag);

        /* Create all MEM SWAP */ 
	int sit;
	for(sit = 0; sit < PAGING_MAX_MMSWP; sit++)
	       init_memphy(&mswp[sit], memswpsz[sit], rdmflag);

	/* In Paging mode, it needs passing the system mem to each PCB through loader*/
	struct mmpaging_ld_args *mm_ld_args = malloc(sizeof(struct mmpaging_ld_args));

	mm_ld_args->timer_id = ld_event;
	mm_ld_args->mram = (struct memphy_struct *) &mram;
	mm_ld_args->mswp = (struct memphy_struct**) &mswp;
	mm_ld_args->active_mswp = (struct memphy_struct *) &mswp[0];
        mm_ld_args->active_mswp_id = 0;
#endif

	/* Init scheduler */
	init_scheduler();

	/* Run CPU and loader */
#ifdef MM_PAGING
	pthread_create(&ld, NULL, ld_routine, (void*)mm_ld_args);
#else
	pthread_create(&ld, NULL, ld_routine, (void*)ld_event);
#endif
	for (i = 0; i < num_cpus; i++) {
		pthread_create(&cpu[i], NULL,
			cpu_routine, (void*)&args[i]);
	}

	/* Wait for CPU and loader finishing */
	for (i = 0; i < num_cpus; i++) {
		pthread_join(cpu[i], NULL);
	}
	pthread_join(ld, NULL);

	/* Stop timer */
	stop_timer();

	return 0;

}



