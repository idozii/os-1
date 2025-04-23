
# Scheduling Mechanism

- Process Priority: Processes with higher priority(lower numerical value) are schedule first.

- Time Slots: Each priority level has allocated slots based on the equation slot[i]=MAX_PRIO-i.

- CPU Availability: Multiple CPUS can run processes concurrently.

# Timing of Operations

- Sometimes, deallocation happens first because memory operations displayed in the order they occur, even if they're from different proceses.

# Calculating Frame Number

## Frame Number Extraction

```C
    #define PAGING_FPN(x) GETVAL(x,PAGING_FPN_MASK,PAGING_ADDR_FPN_LOBIT)
```

- This macro extracts the Frame Physical Number (FPN) from a page table using bit operations.

- The hexadecimal value (like 80000005) is the actual PTE, with the frame number encoded in it according to the defined bit masks in the systems.

## From the Source Code

```C
for (pgit = pgn_start; pgit < pgn_end; pgit++) {
  printf("Page Number: %d -> Frame Number: %d\n", pgit, PAGING_FPN(caller->mm->pgd[pgit]));
}
```

- The Frame Number is displayed during page table printing.

# Memory Management Logic

## System Call Arguments

- a1: System Operation Type
- a2: Physical Address to write/ read
- a3: Value to write/ read

### pg_getpage

- Handles the core page swapping mechanism when a page is not present in physical memory.

```C
struct sc_regs regs;
regs.a1 = vicfpn;       // Source frame number (victim)
regs.a2 = swpfpn;       // Destination frame number (in swap space)
regs.a3 = SYSMEM_SWP_OP; // Operation code for swapping
 
syscall(caller, 17, &regs);
```

### pg_getval

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

### pg_setval

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

## System Call: `killall`

### Process Termination Flow

The `killall` system call terminates all processes with a given name. Here's the flow:

```plaintext
┌───────────────────────────────┐
│ 1. Read process name from mem │
└───────────────┬───────────────┘
                │
                ▼
┌───────────────────────────────┐
│ 2. Search running processes   │◄───────┐
└───────────────┬───────────────┘        │
                │                         │
                ▼                         │
┌───────────────────────────────┐        │
│ 3. Search ready queues        │        │
└───────────────┬───────────────┘        │
                │                         │
                ▼                         │
┌───────────────────────────────┐        │
│ 4. For each matching process: │        │
│    - Remove from queue        │────────┘
│    - Free resources           │
│    - Increment counter        │
└───────────────┬───────────────┘
                │
                ▼
┌───────────────────────────────┐
│ 5. Return terminated count    │
└───────────────────────────────┘
```

# Gantt Chart

## sched input

Time:   | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 |10 |11 |12 |13 |14 |15 |16 |17 |18 |19 |20 |
--------+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
CPU 0   |P1 |P1 |P1 |P1 |   |P3 |P3 |P3 |P3 |   |P3 |P3 |P3 |P3 |   |P3 |   |   |   |   |   |
CPU 1   |   |   |P2 |P2 |P2 |P2 |   |   |   |P2 |P2 |P2 |P2 |   |P1 |   |P1 |P1 |P1 |P1 |   |
--------+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
Process  | Load processes |       Process execution cycle       |      Finishing execution     |
Status   |P1|P2|P3|      |       P1, P2, P3 running            |    P3 ends    |    P1 ends   |

### Explanation 1

- **Start (Time 0-2):**
  - Time 0: Process p1s (priority 1) arrives and starts on CPU 0
  - Time 1: Process p2s (priority 0) arrives
  - Time 2: Process p3s (priority 0) arrives and p2s (higher priority) starts on CPU 1

- **First Execution Cycle (Time 0-7):**
  - P1 runs on CPU 0 for its time slice (4 units)
  - P2 runs on CPU 1 for its time slice (4 units)
  - After P1 completes its time slice, P3 starts on CPU 0

- **Second Execution Cycle (Time 8-15):**
  - P3 continues on CPU 0
  - P2 returns to CPU 1 for another time slice
  - P3 completes another time slice on CPU 0
  - P1 returns to CPU 1 after P2 completes its slots

- **Completion (Time 15-20):**
  - P3 finishes execution on CPU 0 (time ~15)
  - P1 completes execution on CPU 1 (time ~20)
  - P2 completes execution (before P1)

## sched_0 input

Time:   | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 |10 |11 |12 |13 |14 |15 |16 |17 |18 |19 |20 |
--------+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
CPU 0   |s0 |s0 |s0 |s0 |s1 |s1 |s1 |s1 |s1 |s1 |s1 |s1 |s1 |s1 |s0 |s0 |   |   |   |   |   |
--------+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
Process  | s0 loading  |   s1 arrives   |      Process execution      |    Process completion |
Status   |   s0 runs   |  s1 preempts   |   s1 runs multiple slots    | s0 finishes | s1 done |

### Explanation 2

- **Initial Phase (Time 0-4):**
  - Time 0: Process s0 (priority 4) arrives and starts execution
  - s0 runs for its first 2 time slices (time 0-2)
  - s0 continues for another 2 time slices (time 2-4)

- **s1 Preemption (Time 4):**
  - Time 4: Process s1 (priority 0) arrives
  - s1 preempts s0 because it has higher priority

- **s1 Execution (Time 4-14):**
  - s1 executes for its full allocation (priority 0 gets 5 slots)
  - Each slot is 2 time units, so s1 runs for 10 time units (5 slots)
  - During this time, s0 waits in its priority queue

- **s0 Completion (Time 14-16):**
  - After s1 completes its slots, s0 returns to finish execution
  - s0 runs for its remaining 2 time units (1 slot)
  - s0 completes at time 16

- **All Processes Complete (Time 16):**
  - All processes have completed execution
  - CPU becomes idle

## sched_1 input

Time:   | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 |10 |11 |12 |13 |14 |15 |16 |17 |18 |19 |20 |21 |22 |23 |24 |25 |26 |27 |28 |29 |30 |31 |32 |33 |34 |35 |36 |37 |38 |39 |40 |41 |42 |43 |44 |45 |46 |
--------+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
CPU 0   |s0 |s0 |s0 |s0 |s1 |s1 |s1 |s1 |s3 |s3 |s4 |s4 |s2 |s2 |s3 |s3 |s4 |s4 |s2 |s2 |s3 |s3 |s4 |s4 |s2 |s2 |s3 |s3 |s4 |s4 |s3 |s3 |s4 |s4 |s3 |s3 |s0 |s0 |s0 |s0 |s0 |s0 |s0 |s0 |s0 |s0 |
--------+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
Process  | s0  |   s1 arrives  |  s2,s3 arrive  |      Round-robin between priority 0 processes     |  s2 finishes  |  s3 finishes  |  s4 finishes  | s0 completes |
Status   |s0,p4|   s1,p0       |s1,s2,s3 all p0 |            s1,s2,s3 executing                     |               |               |               |              |

### Explanation 3

- **Initial Phase (Time 0-4):**
  - Time 0: Process s0 (priority 4) arrives and starts execution
  - s0 runs for its first 2 time slices (time 0-2)
  - s0 continues for another 2 time slices (time 2-4)

- **First High Priority Process (Time 4-7):**
  - Time 4: Process s1 (priority 0) arrives
  - s1 preempts s0 because it has higher priority
  - s1 runs for 2 time units (time 4-6)
  - Time 6: Process s2 (priority 0) arrives

- **Multiple High Priority Processes (Time 6-10):**
  - s1 continues for its second time slice (time 6-8)
  - Time 7: Process s3 (priority 0) arrives
  - After s1's time slice, s2 gets CPU (time 8-10)

- **Round-Robin Among Priority 0 (Time 10-24):**
  - The three priority 0 processes (s1, s2, s3) execute in round-robin order
  - Each gets 2 time units before being put back in the queue
  - The rotation continues: s3 → s1 → s2 → s3 → s1 → s2...

- **Process Completion (Time 24-35):**
  - Time 24: s1 completes execution
  - Round-robin continues between s2 and s3
  - Time 34: s2 completes execution
  - Time 35: s3 completes execution

- **Low Priority Completion (Time 36-46):**
  - s0 (priority 4) returns to CPU
  - s0 runs for its remaining 10 time units
  - Time 46: s0 completes execution
  - All processes have finished
  