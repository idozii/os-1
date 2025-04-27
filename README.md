# Scheduler

## Scheduler Design

- **Multi-level Queue Scheduling and Round Robin**

-Processes are organized in multiple ready queues based on their priority
-Within each priority level, round-robin scheduling is implemented

- **Process Priority**

-Processes with higher priority (lower numerical value) are scheduled first
-This ensures critical operations receive CPU time before less important ones

- **Time Slot Allocation**

-Each priority level has allocated slots based on the equation: `slot[i] = MAX_PRIO - i`
-Higher priority processes (lower i) receive more consecutive time slots
-This prevents starvation while still prioritizing important processes

- **Multi-CPU Support**

-Multiple CPUs can run processes concurrently
-Improves overall system throughput and responsiveness
-Allows parallel execution of processes from different priority levels

- **Dual Priority Mechanism**

- Each process has a default priority in its description file
- This priority can be overwritten by the configuration specified in the input file
- When a conflict exists, the input configuration priority takes precedence
- This provides flexibility in process management without modifying process files

## Implementation

### Dual Mechanism

```C
// In loader.c - Default priority assignment
#ifdef MLQ_SCHED
proc->prio = DEFAULT_PRIO;
#endif

// In os.c - Priority override during loading
struct pcb_t *proc = load(ld_processes.path[i]);
#ifdef MLQ_SCHED
proc->prio = ld_processes.prio[i];  // Override with configuration priority
#endif
```

### Priority queue

- enqueue():

```C
void enqueue(struct queue_t *queue, struct pcb_t *proc) {
    // Verify valid queue and process
    if (queue == NULL || proc == NULL) return;

    // Check if queue is full
    if (queue->size == MAX_QUEUE_SIZE) {
        return;
    }

    // Add process to the end of the queue
    queue->proc[queue->size] = proc;
    queue->size++;
}
```

- dequeue():

```C
// Remove and return first process from queue
struct pcb_t* dequeue(struct queue_t *queue) {
    if (queue == NULL || queue->size == 0) {
        return NULL;
    }

    struct pcb_t* proc = queue->proc[0];
#ifdef MLQ_SCHED
    // Shift remaining processes forward
    for (int i = 0; i < queue->size - 1; i++) {
        queue->proc[i] = queue->proc[i + 1];
    }

    queue->size--;
    return proc;
#else 
    ...
#endif
}
```

### Schedule

- get_mlq_proc():

```C
struct pcb_t * get_mlq_proc(void) {
    struct pcb_t * proc = NULL;
    pthread_mutex_lock(&queue_lock);
    int i;
    
    // Core MLQ scheduling algorithm
    for (i = 0; i < MAX_PRIO; ++i) {
        // Skip empty queues or those that used up their slots
        if (empty(&mlq_ready_queue[i]) || slot[i] == 0) {
            // Reset slot count when depleted
            slot[i] = MAX_PRIO - i;
            continue;
        }
        
        // Get process from this priority level
        proc = dequeue(&mlq_ready_queue[i]);
        // Decrement remaining slots for this priority
        slot[i]--;
        break;
    }
    
    pthread_mutex_unlock(&queue_lock);
    return proc;
}
```

## Testing

### sched input

```markdown
| Parameter           | Value | Description                            |
| ------------------- | ----- | -------------------------------------- |
| Time slice          | 4     | Each process executes for 4 time units |
| Number of CPUs      | 2     | Two processors running in parallel     |
| Number of processes | 3     | Total processes to be scheduled        |

| Process Information |          |              |                               |
| ------------------- | -------- | ------------ | ----------------------------- |
| **Start Time**      | **Name** | **Priority** | **Notes**                     |
| 0                   | p1s      | 1            | Medium priority, starts first |
| 1                   | p2s      | 0            | High priority, arrives second |
| 2                   | p3s      | 0            | High priority, arrives last   |
```

#### Gantt chart 1

```markdown
Time: | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 |10 |11 |12 |13 |14 |15 |16 |17 |18 |19 |
--------+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
CPU 0 |   |   |P2 |P2 |P2 |P2 |P2 |P2 |P2 |P2 |P2 |P2 |P2 |P1 |P1 |P1 |P1 |P1 |P1 |   |
CPU 1 |   |P1 |P1 |P1 |P1 |P3 |P3 |P3 |P3 |P3 |P3 |P3 |P3 |P3 |P3 |P3 |   |   |   |   |
--------+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
        ⌞────────────⌟⌞────────────────────────────────────────⌟⌞─────────────────⌟
         Load phase           Execution phase               Completion phase
```

#### Explanation 1

- **Time 0-2: Load Phase**
  - Time 0: System initializes, P1 (priority 1) is loaded
  - Time 1: CPU 1 dispatches P1, P2 (priority 0) is loaded
  - Time 2: CPU 0 dispatches P2, P3 (priority 0) is loaded

- **Time 3-12: Execution Phase Part 1**
  - Time 5:
    - P1 completes its time slice on CPU 1 and returns to queue
    - P3 (priority 0) is dispatched on CPU 1
    - P2 (priority 0) is redispatched on CPU 0 after its time slice
  - Time 8:
    - P3 completes its time slice on CPU 1 and returns to queue
    - P3 is redispatched on CPU 1 (round-robin among priority 0)
    - P2 is redispatched on CPU 0 after another time slice

- **Time 13-16: Execution Phase Part 2**
  - Time 13:
    - P2 finishes execution completely on CPU 0
    - P1 (priority 1) is dispatched on CPU 0
    - P3 continues execution on CPU 1
  - Time 16: P3 finishes execution completely on CPU 1, CPU 1 stops

- **Time 17-19: Completion Phase**
  - Time 17: P1 completes another time slice on CPU 0, returns to queue
  - Time 17-19: P1 is redispatched and completes execution
  - Time 19: All processes complete, CPU 0 stops

### sched_0 input

```markdown
| Parameter           | Value | Description                            |
| ------------------- | ----- | -------------------------------------- |
| Time slice          | 2     | Each process executes for 2 time units |
| Number of CPUs      | 1     | Single processor system                |
| Number of processes | 2     | Total processes to be scheduled        |

| Process Information |          |              |                               |
| ------------------- | -------- | ------------ | ----------------------------- |
| **Start Time**      | **Name** | **Priority** | **Notes**                     |
| 0                   | s0       | 4            | Lowest priority, starts first |
| 4                   | s1       | 0            | High priority, arrives second |
```

#### Gantt chart 2

```markdown
Time: | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 |10 |11 |12 |13 |14 |15 |16 |17 |18 |19 |20 |21 |22 |23 |
--------+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
CPU 0 |   |s0 |s0 |s0 |   |s1 |s1 |s1 |s1 |s1 |s1 |s1 |s0 |s0 |s0 |s0 |s0 |s0 |s0 |s0 |s0 |s0 |s0 |s0 |
--------+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
⌞────────⌟ ⌞─────────────────⌟ ⌞───────────────────────────────────⌟
s0 starts  s1 runs (higher    s0 completes execution
          priority)
```

#### Explanation 2

- **Time 0-3:**

- Low priority process s0 starts execution at time 1 (after loading in time 0)
- It runs for 3 time units (slots 1-3)

- **Time 4-11:**

- High priority process s1 arrives at time 4
- s1 preempts s0 and runs from time 5-11 (7 time units)
- s1 completes execution at the end of time 11

- **Time 12-23:**

- s0 returns to finish its remaining execution (12 more time units)
- s0 completes execution at the end of time 23

### sched_1 input

```markdown
| Parameter           | Value | Description                            |
| ------------------- | ----- | -------------------------------------- |
| Time slice          | 2     | Each process executes for 2 time units |
| Number of CPUs      | 1     | Single processor system                |
| Number of processes | 4     | Total processes to be scheduled        |

| Process Information |          |              |                               |
| ------------------- | -------- | ------------ | ----------------------------- |
| **Start Time**      | **Name** | **Priority** | **Notes**                     |
| 0                   | s0       | 4            | Lowest priority, starts first |
| 4                   | s1       | 0            | Highest priority, arrives 2nd |
| 6                   | s2       | 0            | Highest priority, arrives 3rd |
| 7                   | s3       | 0            | Highest priority, arrives 4th |
```

#### Gantt chart 3

```markdown
Time: 0         5         10        15        20        25        30        35        40        45  
      ┌─────────┬─────────┬─────────┬─────────┬─────────┬─────────┬─────────┬─────────┬─────────┐
CPU 0 │    s0   │    s1   │         Round-robin scheduling between          │    s0    │         │
      │         │         │           s1, s2, and s3 (priority 0)           │          │         │
      └─────────┴─────────┴─────────────────────────────────────────────────┴──────────┴─────────┘
        ↑         ↑         ↑                   ↑                             ↑          
        │         │         │                   │                             │          
        │         │         │                   │                             └─ Low priority s0 returns
        │         │         │                   │                                and completes (35-46)
        │         │         │                   │          
        │         │         │                   └─ High priority processes finish
        │         │         │                      s1 @ time 24
        │         │         │                      s2 @ time 34
        │         │         │                      s3 @ time 35
        │         │         │          
        │         │         └─ Round-robin begins among priority 0 processes
        │         │            Each process gets 2 time units before switching
        │         │          
        │         └─ s1 preempts s0 (higher priority)
        │            Process s2 and s3 arrive at times 6 and 7
        │          
        └─ s0 starts execution (low priority)
           Only process available at system start
```

#### Explanation 3

- Time 0-4:
- Process s0 (priority 4) starts execution
  - It's the only process available at this time
  - Executes from time 1-3

- Time 4-8:
- Process s1 (priority 0) arrives at time 4
  - Higher priority than s0, so it preempts s0
  - s1 begins execution at time 5
- Process s2 (priority 0) arrives at time 6
  - Same priority as s1, so it's queued behind s1
- Process s3 (priority 0) arrives at time 7
  - Same priority as s1 and s2, queued after s2

- Time 8-35:
- Round-robin scheduling among high-priority processes
  - Processes s1, s2, and s3 (all priority 0) take turns executing
  - Each gets exactly 2 time units (the configured time slice)
  - Processes complete in order:
    - s1 completes at time 24
    - s2 completes at time 34
    - s3 completes at time 35

- Time 35-46:
- Process s0 (priority 4) resumes execution
  - Low priority process was waiting for all high-priority processes to complete
  - Executes from time 35-46 to completion

# Memory Management

- Physical Memory is finite so that Virtual Memory is created to increase task capacity and OS performance.
- Physical Memory -> frame including RAM & SWAP while Virtual Memory -> page.
- Using mapping from Virtual Memory to Physical Memory.

## Calculating Frame Number

### Frame Number Extraction

```C
    #define PAGING_FPN(x) GETVAL(x,PAGING_FPN_MASK,PAGING_ADDR_FPN_LOBIT)
```

- This macro extracts the Frame Physical Number (FPN) from a page table using bit operations.

- The hexadecimal value (like 80000005) is the actual PTE, with the frame number encoded in it according to the defined bit masks in the systems.

### From the Source Code

```C
for (pgit = pgn_start; pgit < pgn_end; pgit++) {
  printf("Page Number: %d -> Frame Number: %d\n", pgit, PAGING_FPN(caller->mm->pgd[pgit]));
}
```

- The Frame Number is displayed during page table printing.

## Memory Management Logic

### System Call Arguments

- a1: System Operation Type
- a2: Physical Address to write/ read
- a3: Value to write/ read

#### pg_getpage

- Handles the core page swapping mechanism when a page is not present in physical memory.

```C
struct sc_regs regs;
regs.a1 = vicfpn;       // Source frame number (victim)
regs.a2 = swpfpn;       // Destination frame number (in swap space)
regs.a3 = SYSMEM_SWP_OP; // Operation code for swapping

syscall(caller, 17, &regs);
```

#### pg_getval

- Reads a value from memory at a given virtual address.

```C
int pg_getval(struct mm_struct *mm, int addr, BYTE *data, struct pcb_t *caller)
{
  // ...
  struct sc_regs regs;
  regs.a1 = SYSMEM_IO_READ;  // Operation type: read
  regs.a2 = phyaddr;         // Physical address to read from
  regs.a3 = (uint32_t)(*data); // Value read from memory

  syscall(caller, 17, &regs);
  // ...
}
```

#### pg_setval

- Writes a value to memory at a given virtual address.

```C
int pg_setval(struct mm_struct *mm, int addr, BYTE value, struct pcb_t *caller)
{
  // ...
  struct sc_regs regs;
  regs.a1 = SYSMEM_IO_WRITE; // Operation type: write
  regs.a2 = phyaddr;         // Physical address to write to
  regs.a3 = value;           // Value to write to memory

  syscall(caller, 17, &regs);
  // ...
}
```

## swapping Technique

Swapping is a memory management technique that allows the operating system to handle situations where the physical RAM is insufficient to hold all the processes that need to be executed. This critical function enables multitasking and efficient memory utilization in operating systems.

```markdown
┌────────────────┐ ┌────────────────┐ ┌────────────────┐
│ Page Fault │ │ Victim Page │ │ Get Swap Frame │
│ Detection │────▶│ Selection │────▶│ │
└────────────────┘ └────────────────┘ └────────┬───────┘
│
▼
┌────────────────┐ ┌────────────────┐ ┌────────────────┐
│ FIFO Queue │ │ Page Table │ │ Data Transfer │
│ Management │◀────│ Updates │◀────│ │
└────────────────┘ └────────────────┘ └────────────────┘
```

### How it works

1. **Basic Principle**:

    - When physical RAM is insufficient, less frequently used memory pages are temporarily moved to secondary storage (swap space)
    - This frees up RAM for more urgent processes or data
    - Pages can be swapped back into RAM when needed again

2. **Swap Space**:

    - Dedicated area on disk (or other storage) reserved for storing memory pages
    - Can be a separate partition or a swap file
    - In our implementation, multiple swap devices can be configured with different sizes

3. **Page Replacement Algorithm**:
    - Our system uses a FIFO (First-In-First-Out) algorithm to select victim pages
    - The oldest page in memory is selected as the victim to be swapped out
    - This is tracked using the `fifo_pgn` linked list in the `mm_struct`

### Implementation Details

#### Page Table Structure

- Each process has a page table (`pgd` array in `mm_struct`)
- Page table entries (PTEs) contain information about whether a page is:
    - Present in physical memory (indicated by `PAGING_PAGE_PRESENT` bit)
    - Swapped out to disk (containing the swap frame number)

#### Swap Operation Sequence

1. **Page Fault Detection**:

    - When accessing a virtual address, the system checks if the corresponding page is present
    - If not present, a page fault occurs, triggering the swapping mechanism

2. **Victim Selection**:

    - System calls `find_victim_page()` to select a page to be swapped out
    - The selected victim page will give up its physical frame to the requested page

3. **Obtaining Swap Space**:

    - System calls `MEMPHY_get_freefp()` to get a free frame in swap space
    - If swap space is full or not configured, handles the error gracefully

4. **Data Transfer**:

    - Copies data from victim frame in RAM to the allocated swap frame
    - Uses `__swap_cp_page()` to perform the copy operation

5. **Page Table Updates**:

    - Updates the victim's PTE to mark it as not present and records its swap location
    - Updates the faulting page's PTE to mark it as present and assigns the physical frame

6. **FIFO Queue Management**:
    - Adds the newly loaded page to the FIFO queue with `enlist_pgn_node()`
    - Ensures proper tracking for future victim selection

#### System Call Interaction

- Swapping operations use system call 17 (`sys_memmap`)
- The operation type `SYSMEM_SWP_OP` specifies a swap operation
- Arguments include source and destination frame numbers

```c
struct sc_regs regs;
regs.a1 = vicfpn;       // Source frame number (victim)
regs.a2 = swpfpn;       // Destination frame number (in swap space)
regs.a3 = SYSMEM_SWP_OP; // Operation code for swapping

syscall(caller, 17, &regs);
```

### Handling Edge Cases

```markdown
┌─────────────────────────────────────────────────┐
│ Swap Space Unavailability │
├─────────────────────────────────────────────────┤
│ • Check if caller->active_mswp exists/has space │
│ • Direct frame reuse without swapping │
│ • Special PTE updates to maintain consistency │
└─────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────┐
│ Multiple Process Competition │
├─────────────────────────────────────────────────┤
│ • Separate virtual address spaces │
│ • Shared physical frames │
│ • FIFO-based fair allocation │
└─────────────────────────────────────────────────┘
```

#### Swap Space Unavailability

- The implementation includes a check for swap space availability
- If swap space is not configured or is full, the system:
    1. Directly reuses a physical frame without swapping to disk
    2. Marks the victim page as not present
    3. Updates the page table entries accordingly

#### Multiple Process Memory Demands

- When multiple processes compete for limited RAM:
    1. Each process gets its own virtual address space
    2. Physical frames are shared among all processes
    3. Swapping ensures fair allocation based on usage patterns

# SYSCALL

## 1. System Call Table (syscall.c)

- \*\*Definition  
  The `sys_call_table` is an array of strings that maps system call numbers (`nr`) to their corresponding names (`sym`). It is populated using the `syscalltbl.lst` file.

- **Non-Implemented System Call Handler**:  
  The function `__sys_ni_syscall` is invoked when a system call number does not match any implemented system call. It logs an error and lists all available system calls.

- **System Call Dispatcher**:  

  The `syscall` function is the entry point for handling system calls. It:
    - Sets the `orig_ax` register to the system call number.
    - Uses a `switch` statement to invoke the appropriate system call handler based on the number (`nr`).
    - Falls back to `__sys_ni_syscall` if the number is invalid

#### 2. **Listing System Calls (`sys_listsyscall.c`)**

- **Purpose**:

  The `__sys_listsyscall` function allows a process to list all available system calls. It iterates through the `sys_call_table` and prints each entry.

- **Implementation**:

    ```C
    int __sys_listsyscall(struct pcb_t *caller, struct sc_regs* reg) {
        for (int i = 0; i < syscall_table_size; i++)
            printf("%s\n", sys_call_table[i]);
        return 0;
    }
    ```

## 3. Kill All Processes: `killall`

- **Purpose**:  
  The `__sys_killall` function terminates all processes with a specified name. It traverses the running and ready queues, matches processes by name, and removes them.

- **Key Features**:

    - **Process Name Retrieval**:  
      Reads the target process name from memory using `libread`.
    - **Queue Traversal**:  
      Iterates through the `running_list` and `ready_queue` (or `mlq_ready_queue` for multi-level queue scheduling) to find matching processes.
    - **Process Termination**:  
      Removes matching processes from the queues and frees their resources.

- **Implementation Highlights**:

    - Handles both single and multi-level queue schedulers.
    - Logs the termination of each process and the total count of terminated processes.

- Process Termination Flow:

The `killall` system call terminates all processes with a given name. Here's the flow:

```markdown
🕒 Time progression →
┌─────────┬─────────┬─────────┬─────────┬─────────┬─────────┬─────────┐
│ t0      │ t1      │ t2      │ t3      │ t4      │ t5      │ t6      │
├─────────┼─────────┼─────────┼─────────┼─────────┼─────────┼─────────┤
│Initialize│ Read    │ Search  │ Search  │ Remove  │ Free    │ Return │
│ killall()│ process │ running │ ready   │processes│resources│ count  │
│parameters│ name    │processes│ queues  │         │         │        │
└─────┬────┴────┬────┴────┬────┴────┬────┴────┬────┴────┬────┴────┬───┘
      │         │         │         │         │         │         │
      ▼         │         │         │         │         │         │
┌──────────┐    │         │         │         │         │         │
│ \__sys_  │    │         │         │         │         │         │
│ killall()│    │         │         │         │         │         │
└─────┬────┘    │         │         │         │         │         │
      │         │         │         │         │         │         │
      ▼         │         │         │         │         │         │
┌──────────┐    │         │         │         │         │         │
│ libread() │────▶        │         │         │         │         │
│(get name) │             │         │         │         │         │
└───────────┘             │         │         │         │         │
                          │         │         │         │         │
                          ▼         │         │         │         │
                ┌──────────────┐    │         │         │         │
                │Check running │────▶         │         │         │
                │ process list │              │         │         │
                └──────────────┘              │         │         │
                                              │         │         │
                                              ▼         │         │
                                    ┌──────────┐        │         │
                                    │ Check    │────▶   │         │
                                    │all queues│        │         │
                                    └──────────┘        │         │
                                                        │         │
                                                        ▼         │
                                            ┌───────────────┐     │
                                            │For each match:│     │
                                            │• Remove       │     │
                                            │• Free mem     │     │
                                            │• Count++      │     │
                                            └───────┬───────┘     │
                                                    │             │
                                                    ▼             │
                                            ┌───────────────┐     │
                                            │Return number  │     │
                                            │of terminated  │     │
                                            │processes      │     │
                                            └───────────────┘     │
```
