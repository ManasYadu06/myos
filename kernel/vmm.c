#include "vmm.h"
#include "pmm.h"
#include "vga.h"

/* -----------------------------------------------------------------------
 * The kernel's page directory — aligned to 4KB (CPU requirement)
 * Declared static so it lives in .bss, zeroed at boot
 * ----------------------------------------------------------------------- */
static page_directory_t kernel_dir __attribute__((aligned(4096)));

/* -----------------------------------------------------------------------
 * Helper: extract indices from a virtual address
 * ----------------------------------------------------------------------- */
#define PD_INDEX(virt)  ((virt) >> 22)          /* top 10 bits    */
#define PT_INDEX(virt)  (((virt) >> 12) & 0x3FF) /* middle 10 bits */

/* -----------------------------------------------------------------------
 * vmm_map_page — map one virtual page to one physical page
 *
 * 1. Find the page directory entry using top 10 bits of virt
 * 2. If no page table exists for that entry, allocate one via PMM
 * 3. Fill the page table entry with phys | flags
 * ----------------------------------------------------------------------- */
void vmm_map_page(uint32_t virt, uint32_t phys, uint32_t flags) {
    uint32_t pd_idx = PD_INDEX(virt);
    uint32_t pt_idx = PT_INDEX(virt);

    page_table_t *table;

    if (kernel_dir.entries[pd_idx] & PAGE_PRESENT) {
        /* page table already exists — get its address */
        table = (page_table_t *)(kernel_dir.entries[pd_idx] & ~0xFFF);
    } else {
        /* allocate a new page table from PMM */
        table = (page_table_t *)pmm_alloc_page();
        if (!table) {
            vga_print("[VMM] Out of memory for page table!\n");
            return;
        }
        /* zero the new table */
        for (int i = 0; i < PAGE_TABLE_SIZE; i++)
            table->entries[i] = 0;

        /* install into page directory */
        kernel_dir.entries[pd_idx] = (uint32_t)table | PAGE_PRESENT | PAGE_WRITABLE;
    }

    /* set the page table entry */
    table->entries[pt_idx] = (phys & ~0xFFF) | (flags & 0xFFF);

    /* flush TLB for this virtual address */
    __asm__ volatile("invlpg (%0)" :: "r"(virt) : "memory");
}

/* -----------------------------------------------------------------------
 * vmm_unmap_page — remove a virtual → physical mapping
 * ----------------------------------------------------------------------- */
void vmm_unmap_page(uint32_t virt) {
    uint32_t pd_idx = PD_INDEX(virt);
    uint32_t pt_idx = PT_INDEX(virt);

    if (!(kernel_dir.entries[pd_idx] & PAGE_PRESENT)) return;

    page_table_t *table = (page_table_t *)(kernel_dir.entries[pd_idx] & ~0xFFF);
    table->entries[pt_idx] = 0;

    __asm__ volatile("invlpg (%0)" :: "r"(virt) : "memory");
}

/* -----------------------------------------------------------------------
 * vmm_is_mapped — check if a virtual address has a valid mapping
 * ----------------------------------------------------------------------- */
int vmm_is_mapped(uint32_t virt) {
    uint32_t pd_idx = PD_INDEX(virt);
    uint32_t pt_idx = PT_INDEX(virt);

    if (!(kernel_dir.entries[pd_idx] & PAGE_PRESENT)) return 0;

    page_table_t *table = (page_table_t *)(kernel_dir.entries[pd_idx] & ~0xFFF);
    return (table->entries[pt_idx] & PAGE_PRESENT) ? 1 : 0;
}

/* -----------------------------------------------------------------------
 * vmm_init — identity map the first 4MB then enable paging
 *
 * Identity map = virtual address == physical address for the kernel.
 * This keeps existing code/data pointers valid after paging is enabled.
 * We map 4MB (1024 pages) which covers the kernel + early memory.
 *
 * Steps:
 *   1. Identity map 0x00000000 → 0x003FFFFF (4MB)
 *   2. Load page directory address into CR3
 *   3. Set bit 31 of CR0 to enable paging
 * ----------------------------------------------------------------------- */
void vmm_init(void) {
    /* identity map first 4MB — 1024 pages of 4KB each */
    for (uint32_t i = 0; i < 1088; i++)
        vmm_map_page(i * 0x1000, i * 0x1000, PAGE_PRESENT | PAGE_WRITABLE);

    /* load page directory into CR3 (PDBR register) */
    __asm__ volatile("mov %0, %%cr3" :: "r"((uint32_t)&kernel_dir) : "memory");

    /* enable paging by setting bit 31 of CR3 */
    uint32_t cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= (1 << 31);
    __asm__ volatile("mov %0, %%cr0" :: "r"(cr0) : "memory");
}
