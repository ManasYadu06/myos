#ifndef HEAP_H
#define HEAP_H

#include <stdint.h>
#include <stddef.h>

/* -----------------------------------------------------------------------
 * Heap layout
 *
 * HEAP_START : first virtual address handed to the heap
 *              must be mapped by vmm before heap_init() is called
 * HEAP_SIZE  : initial pool size (256 KB — 64 pages from PMM)
 *
 * Free-list node sits at the START of every free block:
 *
 *   [ heap_block_t header | .... usable bytes .... ]
 *
 * Allocated block: header.magic = HEAP_MAGIC_USED, no next pointer needed
 * Free block     : header.magic = HEAP_MAGIC_FREE, next → next free block
 * ----------------------------------------------------------------------- */
#define HEAP_START      0x00400000UL   /* pick above kernel + vmm mappings */
#define HEAP_SIZE       (256 * 1024)   /* 256 KB initial pool              */

#define HEAP_MAGIC_FREE 0xFEEDBEEFUL
#define HEAP_MAGIC_USED 0xDEADC0DEUL

typedef struct heap_block {
    uint32_t          magic;   /* HEAP_MAGIC_FREE or HEAP_MAGIC_USED */
    uint32_t          size;    /* usable bytes AFTER this header      */
    struct heap_block *next;   /* next free block (free list only)    */
} heap_block_t;

/* -----------------------------------------------------------------------
 * Public API
 * ----------------------------------------------------------------------- */

/* map HEAP_SIZE bytes at HEAP_START via PMM+VMM, build initial free block */
void  heap_init(void);

/* allocate at least `size` bytes; returns NULL on failure */
void *kmalloc(size_t size);

/* free a pointer returned by kmalloc */
void  kfree(void *ptr);

/* diagnostics — counts free/used blocks and byte totals */
void  heap_stats(uint32_t *free_bytes, uint32_t *used_bytes,
                 uint32_t *free_blocks, uint32_t *used_blocks);

#endif /* HEAP_H */
