#include "pmm.h"
#include "vga.h"

/*
 * Multiboot memory map structures
 * GRUB passes a pointer to multiboot_info_t in EBX (we saved it as mb_info)
 */
typedef struct __attribute__((packed)) {
    uint32_t size;
    uint32_t addr_low;
    uint32_t addr_high;
    uint32_t len_low;
    uint32_t len_high;
    uint32_t type;      /* 1 = available RAM, anything else = reserved */
} mmap_entry_t;

typedef struct __attribute__((packed)) {
    uint32_t flags;
    uint32_t mem_lower;   /* KB below 1MB */
    uint32_t mem_upper;   /* KB above 1MB */
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
    uint32_t syms[4];
    uint32_t mmap_length;
    uint32_t mmap_addr;
} multiboot_info_t;

/* -----------------------------------------------------------------------
 * Bitmap allocator
 * 1 bit per 4KB page. Bit=0 means free, bit=1 means used.
 * Supports up to 128MB (32768 pages = 1024 uint32_t entries)
 * ----------------------------------------------------------------------- */
#define MAX_PAGES  32768
#define BITMAP_SIZE (MAX_PAGES / 32)

static uint32_t bitmap[BITMAP_SIZE];
static uint32_t total_pages = 0;
static uint32_t used_pages  = 0;

static void bitmap_set(uint32_t page) {
    bitmap[page / 32] |=  (1 << (page % 32));
}
static void bitmap_clear(uint32_t page) {
    bitmap[page / 32] &= ~(1 << (page % 32));
}
static int bitmap_test(uint32_t page) {
    return bitmap[page / 32] & (1 << (page % 32));
}

/* -----------------------------------------------------------------------
 * pmm_init — parse Multiboot memory map, mark usable pages as free
 * ----------------------------------------------------------------------- */
void pmm_init(uint32_t mb_info_addr) {
    multiboot_info_t *mb = (multiboot_info_t *)mb_info_addr;

    /* mark everything used by default */
    for (int i = 0; i < BITMAP_SIZE; i++)
        bitmap[i] = 0xFFFFFFFF;

    /* check mmap flag (bit 6) */
    if (!(mb->flags & (1 << 6))) {
        vga_set_color(VGA_LIGHT_RED, VGA_BLACK);
        vga_print("[PMM] No memory map from GRUB!\n");
        return;
    }

    uint32_t mmap_end = mb->mmap_addr + mb->mmap_length;
    uint32_t entry_addr = mb->mmap_addr;

    while (entry_addr < mmap_end) {
        mmap_entry_t *entry = (mmap_entry_t *)entry_addr;

        if (entry->type == 1) {
            /* available RAM region */
            uint32_t start = entry->addr_low;
            uint32_t len   = entry->len_low;
            uint32_t page  = start / PAGE_SIZE;
            uint32_t count = len   / PAGE_SIZE;

            for (uint32_t i = 0; i < count && page + i < MAX_PAGES; i++) {
                bitmap_clear(page + i);
                total_pages++;
            }
        }
        entry_addr += entry->size + sizeof(entry->size);
    }

    /* re-mark page 0 as used (avoid null pointer allocations) */
    bitmap_set(0);

    /* mark kernel pages as used (0x100000 to ~0x200000, safe estimate) */
    for (uint32_t p = 0x100 ; p < 0x200; p++)
        bitmap_set(p);

    used_pages = 0;
    for (int i = 0; i < BITMAP_SIZE; i++)
        for (int b = 0; b < 32; b++)
            if (bitmap[i] & (1 << b)) used_pages++;
}

/* -----------------------------------------------------------------------
 * pmm_alloc_page — find first free page, mark used, return address
 * ----------------------------------------------------------------------- */
void *pmm_alloc_page(void) {
    for (uint32_t i = 0; i < BITMAP_SIZE; i++) {
        if (bitmap[i] == 0xFFFFFFFF) continue;  /* all used, skip */
        for (int b = 0; b < 32; b++) {
            uint32_t page = i * 32 + b;
            if (!bitmap_test(page)) {
                bitmap_set(page);
                used_pages++;
                return (void *)(page * PAGE_SIZE);
            }
        }
    }
    return 0;  /* out of memory */
}

/* -----------------------------------------------------------------------
 * pmm_free_page — mark page as free
 * ----------------------------------------------------------------------- */
void pmm_free_page(void *addr) {
    uint32_t page = (uint32_t)addr / PAGE_SIZE;
    if (bitmap_test(page)) {
        bitmap_clear(page);
        used_pages--;
    }
}

uint32_t pmm_free_pages(void)  { return total_pages - used_pages; }
uint32_t pmm_total_pages(void) { return total_pages; }
