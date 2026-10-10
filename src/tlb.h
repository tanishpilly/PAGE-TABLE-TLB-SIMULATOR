#ifndef TLB_H
#define TLB_H

#include <stdbool.h>
#include <stdint.h>

/**
 * Structure representing a single TLB entry.
 */
typedef struct {
    int page_num;        /* Virtual page number */
    int frame_num;       /* Physical frame number */
    bool valid;          /* Valid flag */
    uint32_t last_used;  /* Access timestamp for LRU replacement */
} TLBEntry;

/**
 * Structure representing the Translation Lookaside Buffer (TLB).
 */
typedef struct {
    TLBEntry *entries;
    int size;            /* Maximum number of TLB entries */
    uint32_t counter;    /* Counter to track access order */
} TLB;

/**
 * Initialize TLB with the specified size.
 */
TLB* tlb_init(int size);

/**
 * Free resources allocated for the TLB.
 */
void tlb_free(TLB *tlb);

/**
 * Search for page_num in TLB.
 * Returns true on TLB HIT and populates *frame_out.
 * Returns false on TLB MISS.
 */
bool tlb_lookup(TLB *tlb, int page_num, int *frame_out);

/**
 * Insert or update a mapping in the TLB using LRU replacement.
 */
void tlb_insert(TLB *tlb, int page_num, int frame_num);

/**
 * Invalidate all entries in the TLB.
 */
void tlb_flush(TLB *tlb);

/**
 * Display current status of the TLB.
 */
void tlb_display(const TLB *tlb);

#endif /* TLB_H */
