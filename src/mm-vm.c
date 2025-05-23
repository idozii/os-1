// #ifdef MM_PAGING
/*
 * PAGING based Memory Management
 * Virtual memory module mm/mm-vm.c
 */

#include "string.h"
#include "mm.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

/*get_vma_by_num - get vm area by numID
*@mm: memory region
*@vmaid: ID vm area to alloc memory region
*
*/
struct vm_area_struct *get_vma_by_num(struct mm_struct *mm, int vmaid)
{
  struct vm_area_struct *pvma = mm->mmap;

  if (mm->mmap == NULL)
    return NULL;

  int vmait = pvma->vm_id;

  while (vmait < vmaid)
  {
    pvma = pvma->vm_next;
    if (pvma == NULL)
      return NULL;

    vmait = pvma->vm_id;
  }

  return pvma;
}

int __mm_swap_page(struct pcb_t *caller, int vicfpn, int swpfpn)
{
  if (caller == NULL || caller->mram == NULL || caller->active_mswp == NULL)
  {
    return -1;
  }
  if (vicfpn < 0 || swpfpn < 0)
  {
    return -1;
  }
  __swap_cp_page(caller->mram, vicfpn, caller->active_mswp, swpfpn);
  return 0;
}

/*get_vm_area_node - get vm area for a number of pages
*@caller: caller
*@vmaid: ID vm area to alloc memory region
*@incpgnum: number of page
*@vmastart: vma end
*@vmaend: vma end
*
*/
struct vm_rg_struct *get_vm_area_node_at_brk(struct pcb_t *caller, int vmaid, int size, int alignedsz)
{
  struct vm_rg_struct *newrg;
  /* TODO retrive current vma to obtain newrg, current comment out due to compiler redundant warning*/
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);

  newrg = malloc(sizeof(struct vm_rg_struct));

  /* TODO: update the newrg boundary
  // newrg->rg_start = ...
  // newrg->rg_end = ...
  */
  newrg->rg_start = cur_vma->sbrk;
  newrg->rg_end = cur_vma->sbrk + size;

  return newrg;
}

/*validate_overlap_vm_area
*@caller: caller
*@vmaid: ID vm area to alloc memory region
*@vmastart: vma end
*@vmaend: vma end
*
*/
int validate_overlap_vm_area(struct pcb_t *caller, int vmaid, int vmastart, int vmaend)
{
  struct vm_area_struct *vma = caller->mm->mmap;

  /* TODO validate the planned memory area is not overlapped */
  if (vmastart >= vmaend)
  {
    return -1;
  }

  struct vm_area_struct *cur_area = get_vma_by_num(caller->mm, vmaid);
  if (cur_area == NULL)
  {
    return -1;
  }

  while (vma != NULL)
  {
    if (vma != cur_area && OVERLAP(cur_area->vm_start, cur_area->vm_end, vma->vm_start, vma->vm_end))
    {
      return -1;
    }
    vma = vma->vm_next;
  }
  return 0;
}

/*inc_vma_limit - increase vm area limits to reserve space for new variable
*@caller: caller
*@vmaid: ID vm area to alloc memory region
*@inc_sz: increment size
*
*/
int inc_vma_limit(struct pcb_t *caller, int vmaid, int inc_sz)
{
  int inc_amt = PAGING_PAGE_ALIGNSZ(inc_sz);
  int incnumpage = inc_amt / PAGING_PAGESZ;
  struct vm_rg_struct *area = get_vm_area_node_at_brk(caller, vmaid, inc_sz, inc_amt);
  if (area == NULL)
  {
    return -1;
  }

  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);
  if (cur_vma == NULL)
  {
    return -1;
  }

  struct vm_rg_struct *newrg = malloc(sizeof(struct vm_rg_struct));
  if (newrg == NULL)
  {
    free(area);
    return -1;
  }

  int old_end = cur_vma->vm_end;

  /*Validate overlap of obtained region */
  if (validate_overlap_vm_area(caller, vmaid, area->rg_start, area->rg_end) < 0)
  {
    return -1; /*Overlap and failed allocation */
  }

  /* TODO: Obtain the new vm area based on vmaid */
  cur_vma->vm_end += inc_sz;
  cur_vma->sbrk += inc_sz;

  // Enlist the newrg to the free list
  struct vm_rg_struct *inc_rg = malloc(sizeof(struct vm_rg_struct));
  inc_rg->rg_start = old_end;
  inc_rg->rg_end = cur_vma->vm_end;
  inc_rg->rg_next = NULL;
  enlist_vm_freerg_list(caller->mm, inc_rg);
  
  if (vm_map_ram(caller, area->rg_start, area->rg_end,
                old_end, incnumpage, newrg) < 0)
  {
    return -1; /* Map the memory to MEMRAM */
  }

  return 0;
}

// #endif