#include "tlb.h"
#include <stdio.h>
#include <stdlib.h>

TLB* tlb_init(int size) {
    if (size <= 0) return NULL;
    TLB *tlb = (TLB*)malloc(sizeof(TLB));
    if (!tlb) return NULL;

    tlb->size = size;
    tlb->counter = 0;
    tlb->entries = (TLBEntry*)malloc(sizeof(TLBEntry) * (size_t)size);
    if (!tlb->entries) {
        free(tlb);
        return NULL;
    }

    tlb_flush(tlb);
    return tlb;
}

void tlb_free(TLB *tlb) {
    if (tlb) {
        if (tlb->entries) {
            free(tlb->entries);
        }
        free(tlb);
    }
}

bool tlb_lookup(TLB *tlb, int page_num, int *frame_out) {
    if (!tlb || page_num < 0) return false;

    for (int i = 0; i < tlb->size; i++) {
        if (tlb->entries[i].valid && tlb->entries[i].page_num == page_num) {
            tlb->counter++;
            tlb->entries[i].last_used = tlb->counter;
            if (frame_out) {
                *frame_out = tlb->entries[i].frame_num;
            }
            return true; /* TLB HIT */
        }
    }

    return false; /* TLB MISS */
}

void tlb_insert(TLB *tlb, int page_num, int frame_num) {
    if (!tlb || page_num < 0) return;

    tlb->counter++;

    /* Case 1: Entry already present in TLB, update frame & timestamp */
    for (int i = 0; i < tlb->size; i++) {
        if (tlb->entries[i].valid && tlb->entries[i].page_num == page_num) {
            tlb->entries[i].frame_num = frame_num;
            tlb->entries[i].last_used = tlb->counter;
            return;
        }
    }

    /* Case 2: Unused entry available */
    for (int i = 0; i < tlb->size; i++) {
        if (!tlb->entries[i].valid) {
            tlb->entries[i].page_num = page_num;
            tlb->entries[i].frame_num = frame_num;
            tlb->entries[i].valid = true;
            tlb->entries[i].last_used = tlb->counter;
            return;
        }
    }

    /* Case 3: TLB is full -> LRU replacement */
    int lru_index = 0;
    uint32_t min_used = tlb->entries[0].last_used;
    for (int i = 1; i < tlb->size; i++) {
        if (tlb->entries[i].last_used < min_used) {
            min_used = tlb->entries[i].last_used;
            lru_index = i;
        }
    }

    tlb->entries[lru_index].page_num = page_num;
    tlb->entries[lru_index].frame_num = frame_num;
    tlb->entries[lru_index].valid = true;
    tlb->entries[lru_index].last_used = tlb->counter;
}

void tlb_flush(TLB *tlb) {
    if (!tlb) return;
    tlb->counter = 0;
    for (int i = 0; i < tlb->size; i++) {
        tlb->entries[i].page_num = -1;
        tlb->entries[i].frame_num = -1;
        tlb->entries[i].valid = false;
        tlb->entries[i].last_used = 0;
    }
}

void tlb_display(const TLB *tlb) {
    if (!tlb) return;
    printf("\n========================================\n");
    printf("                  TLB                   \n");
    printf("========================================\n");
    printf("%-10s %-12s %-12s %-10s\n", "Entry", "Page", "Frame", "Status");
    printf("----------------------------------------\n");
    for (int i = 0; i < tlb->size; i++) {
        if (tlb->entries[i].valid) {
            printf("%-10d %-12d %-12d %-10s\n", i, tlb->entries[i].page_num, tlb->entries[i].frame_num, "VALID");
        } else {
            printf("%-10d %-12s %-12s %-10s\n", i, "-", "-", "EMPTY");
        }
    }
    printf("----------------------------------------\n");
}
