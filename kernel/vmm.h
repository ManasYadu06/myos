#ifndef VMM_H
#define VMM_H

#include <stdint.h>

/* -----------------------------------------------------------------------
 * Page entry flags (same bits for both directory and table entries)
 * ----------------------------------------------------------------------- */
#define PAGE_PRESENT    (1 << 0)   /* page is in memory            */
#define PAGE_WRITABLE   (1 << 1)   /* page is writable             */
#define PAGE_USER       (1 << 2)   /* accessible from ring 3       */
#define PAGE_ACCESSED   (1 << 5)   /* CPU sets when page accessed  */
#define PAGE_DIRTY      (1 << 6)   /* CPU sets when page written   */

/* -----------------------------------------------------------------------
 * A page directory has 1024 entries.
 * A page table  has 1024 entries.
 * Each entry is a 32-bit value:
 *   Bits 31-12 : physical address of page table / page (aligned to 4KB)
 *   Bits 11-0  : flags (Present, Writable, User, etc.)
 * ----------------------------------------------------------------------- */
#define PAGE_DIR_SIZE   1024
#define PAGE_TABLE_SIZE 1024

typedef uint32_t page_entry_t;

/* A full page directory */
typedef struct __attribute__((aligned(4096))) {
    page_entry_t entries[PAGE_DIR_SIZE];
} page_directory_t;

/* A full page table */
typedef struct __attribute__((aligned(4096))) {
    page_entry_t entries[PAGE_TABLE_SIZE];
} page_table_t;

/* -----------------------------------------------------------------------
 * Public API
 * ----------------------------------------------------------------------- */
void  vmm_init(void);
void  vmm_map_page(uint32_t virt, uint32_t phys, uint32_t flags);
void  vmm_unmap_page(uint32_t virt);
int   vmm_is_mapped(uint32_t virt);

#endif
