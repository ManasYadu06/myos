#include "heap.h"
#include "pmm.h"
#include "vmm.h"
/* -----------------------------------------------------------------------
 * Free-list allocator
 *
 * HEAP_START (0x00400000) is identity-mapped by vmm_init() which now
 * covers 1088 pages (4MB + 256KB). heap_init() just sets up the free
 * block — no vmm_map_page calls needed.
 *
 * Alignment: every allocation rounded up to 4 bytes.
 * Split on alloc, insert-at-head + coalesce-adjacent on free.
 * ----------------------------------------------------------------------- */

#define ALIGN4(x)  (((x) + 3) & ~3UL)
#define HDR_SIZE   sizeof(heap_block_t)   /* 12 bytes on 32-bit */
#define MIN_SPLIT  (HDR_SIZE + 4)         /* min leftover to justify split */

static heap_block_t *free_list = (heap_block_t *)0;

/* -----------------------------------------------------------------------
 * heap_init()
 * Region is already identity-mapped — just initialise one giant free block.
 * Must be called after vmm_init().
 * ----------------------------------------------------------------------- */
void heap_init(void) {
    free_list        = (heap_block_t *)HEAP_START;
    free_list->magic = HEAP_MAGIC_FREE;
    free_list->size  = HEAP_SIZE - HDR_SIZE;
    free_list->next  = (heap_block_t *)0;
}

/* -----------------------------------------------------------------------
 * kmalloc() — first-fit with block splitting
 * ----------------------------------------------------------------------- */
void *kmalloc(size_t size) {
    if (!size) return (void *)0;

    uint32_t need = (uint32_t)ALIGN4(size);

    heap_block_t *prev = (heap_block_t *)0;
    heap_block_t *cur  = free_list;

    while (cur) {
        if (cur->magic != HEAP_MAGIC_FREE)
            return (void *)0;   /* heap corruption */

        if (cur->size >= need) {
            if (cur->size >= need + MIN_SPLIT) {
                heap_block_t *split = (heap_block_t *)
                    ((uint8_t *)cur + HDR_SIZE + need);
                split->magic = HEAP_MAGIC_FREE;
                split->size  = cur->size - need - HDR_SIZE;
                split->next  = cur->next;
                cur->size    = need;
                cur->next    = split;
            }

            if (prev) prev->next = cur->next;
            else      free_list  = cur->next;

            cur->magic = HEAP_MAGIC_USED;
            cur->next  = (heap_block_t *)0;
            return (void *)((uint8_t *)cur + HDR_SIZE);
        }

        prev = cur;
        cur  = cur->next;
    }

    return (void *)0;   /* OOM */
}

/* -----------------------------------------------------------------------
 * kfree() — validate, mark free, insert at head, coalesce adjacent
 * ----------------------------------------------------------------------- */
void kfree(void *ptr) {
    if (!ptr) return;

    heap_block_t *blk = (heap_block_t *)((uint8_t *)ptr - HDR_SIZE);

    if (blk->magic != HEAP_MAGIC_USED) return;   /* double-free / bad ptr */

    blk->magic = HEAP_MAGIC_FREE;
    blk->next  = free_list;
    free_list  = blk;

    /* coalesce physically adjacent free blocks */
    heap_block_t *a = free_list;
    while (a && a->next) {
        heap_block_t *b = a->next;
        if ((uint8_t *)a + HDR_SIZE + a->size == (uint8_t *)b) {
            a->size += HDR_SIZE + b->size;
            a->next  = b->next;
        } else {
            a = a->next;
        }
    }
}

/* -----------------------------------------------------------------------
 * heap_stats() — linear scan through pool, tally free/used blocks
 * ----------------------------------------------------------------------- */
void heap_stats(uint32_t *free_bytes, uint32_t *used_bytes,
                uint32_t *free_blocks, uint32_t *used_blocks) {
    *free_bytes  = 0;
    *used_bytes  = 0;
    *free_blocks = 0;
    *used_blocks = 0;

    heap_block_t *cur = (heap_block_t *)HEAP_START;
    uint8_t      *end = (uint8_t *)HEAP_START + HEAP_SIZE;

    while ((uint8_t *)cur < end) {
        if (cur->magic == HEAP_MAGIC_FREE) {
            (*free_blocks)++;
            *free_bytes += cur->size;
        } else if (cur->magic == HEAP_MAGIC_USED) {
            (*used_blocks)++;
            *used_bytes += cur->size;
        } else {
            break;   /* corruption — stop scan */
        }
        cur = (heap_block_t *)((uint8_t *)cur + HDR_SIZE + cur->size);
    }
}
