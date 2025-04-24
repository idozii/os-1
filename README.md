
# Scheduling Mechanism

- Use Multi-level queue Scheduling and Round Robin.

- Process Priority: Processes with higher priority(lower numerical value) are schedule first.

- Time Slots: Each priority level has allocated slots based on the equation slot[i]=MAX_PRIO-i.

- CPU Availability: Multiple CPUS can run processes concurrently.

## Timing of Operations

- Sometimes, deallocation happens first because memory operations displayed in the order they occur, even if they're from different proceses.

# Paging-based Memory Management

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
┌────────────────┐     ┌────────────────┐     ┌────────────────┐
│  Page Fault    │     │ Victim Page    │     │ Get Swap Frame │
│  Detection     │────▶│ Selection      │────▶│                │
└────────────────┘     └────────────────┘     └────────┬───────┘
                                                       │
                                                       ▼
┌────────────────┐     ┌────────────────┐     ┌────────────────┐
│ FIFO Queue     │     │ Page Table     │     │ Data Transfer  │
│ Management     │◀────│ Updates        │◀────│                │
└────────────────┘     └────────────────┘     └────────────────┘
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

# System Call: `killall`

## Process Termination Flow

The `killall` system call terminates all processes with a given name. Here's the flow:

```plaintext
   🕒 Time progression →
┌─────────┬─────────┬─────────┬─────────┬─────────┬─────────┬─────────┐
│    t0   │    t1   │    t2   │    t3   │    t4   │    t5   │    t6   │
├─────────┼─────────┼─────────┼─────────┼─────────┼─────────┼─────────┤
│Initialize│  Read   │ Search  │ Search  │ Remove  │  Free   │ Return  │
│ killall()│ process │ running │ ready   │processes│resources │  count  │
│parameters│  name   │processes│ queues  │         │         │         │
└─────┬────┴────┬────┴────┬────┴────┬────┴────┬────┴────┬────┴────┬────┘
      │         │         │         │         │         │         │
      ▼         │         │         │         │         │         │
┌──────────┐    │         │         │         │         │         │
│  __sys_  │    │         │         │         │         │         │
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
                     ┌──────────────┐         │         │         │
                     │Check running │─────────▶         │         │
                     │ process list │                   │         │
                     └──────────────┘                   │         │
                                              │         │         │
                                              ▼         │         │
                                        ┌──────────┐    │         │
                                        │  Check   │────▶         │
                                        │all queues│              │
                                        └──────────┘              │
                                                                  │
                                                                  ▼
                                                          ┌───────────────┐
                                                          │For each match:│
                                                          │• Remove       │
                                                          │• Free mem     │
                                                          │• Count++      │
                                                          └───────┬───────┘
                                                                  │
                                                                  ▼
                                                          ┌───────────────┐
                                                          │Return number  │
                                                          │of terminated  │
                                                          │processes      │
                                                          └───────────────┘
```

# Gantt Chart

## sched input

```markdown
Time:   | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 |10 |11 |12 |13 |14 |15 |16 |17 |18 |19 |20 |
--------+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
CPU 0   |P1 |P1 |P1 |P1 |   |P3 |P3 |P3 |P3 |   |P3 |P3 |P3 |P3 |   |P3 |   |   |   |   |   |
CPU 1   |   |   |P2 |P2 |P2 |P2 |   |   |   |P2 |P2 |P2 |P2 |   |P1 |   |P1 |P1 |P1 |P1 |   |
--------+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
         ⌞────────────⌟    ⌞────────────────────────────────────⌟    ⌞─────────────────────⌟
         Load phase         Execution phase                           Completion phase
```

### Explanation 1

- **Time 0-2:**
  - P1 starts on CPU0, P2 arrives.

- **Time 2-4:**
  - P3 arrives, P2 gets CPU1 (higher priority).

- **Time 4-8:**
  - P1 completes time slice, P3 starts on CPU0.

- **Time 8-14:**
  - Round-robin continues with P2 on CPU1, P3 on CPU0.

- **Time 14-16:**
  - P1 returns to CPU1 after P2 completes its slots.

- **Time 16-20:**
  - P3 finishes on CPU0, P1 completes on CPU1.

## sched_0 input

```markdown
Time:   | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 |10 |11 |12 |13 |14 |15 |16 |
--------+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
CPU 0   |s0 |s0 |s0 |s0 |s1 |s1 |s1 |s1 |s1 |s1 |s1 |s1 |s1 |s1 |s0 |s0 |   |
--------+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
         ⌞────────⌟  ⌞────────────────────────────────⌟  ⌞─────⌟
         s0 starts    s1 preempts (higher priority)      Completion
```

### Explanation 2

- **Time 0-4:**
  - Low priority process s0 starts execution.

- **Time 4:**
  - High priority process s1 arrives and preempts s0.

- **Time 4-14:**
  - s1 executes for full allocation (5 slots x 2 units).

- **Time 14-16:**
  - s0 returns to finish remaining execution.

- **Time 16:**
  - All processes have completed execution.

## sched_1 input

```markdown
Time:    0    5   10   15   20   25   30   35   40   45   
       ┌────┬────┬────┬────┬────┬────┬────┬────┬────┬────┐
CPU 0  │s0  │s1  │    Round-robin between    │    s0     │
       │    │    │    s1, s2, and s3 (p0)    │completes  │
       └────┴────┴────────────────────────────┴──────────┘
         ↑    ↑    ↑                 ↑         ↑
         │    │    │                 │         └─ Low priority s0 returns
         │    │    │                 └─ s1,s2,s3 finish (time ~35)
         │    │    └─ Round-robin begins among priority 0 processes
         │    └─ s1 preempts s0 (higher priority)
         └─ s0 starts (low priority)
```

### Explanation 3

- **Time 0-4:**
  - s0 (priority 4) starts execution.

- **Time 4-8:**
  - s1 (priority 0) arrives and preempts s0.

- **Time 6-8:**
  - s2 and s3 (priority 0) both arrive.

- **Time 8-35:**
  - All priority 0 processes execute in rotation.

- **Time 36-46:**
  - Low priority s0 returns and finishes.
