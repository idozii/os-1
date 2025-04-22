
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
