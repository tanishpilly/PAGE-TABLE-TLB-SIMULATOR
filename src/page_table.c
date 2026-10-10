#include "page_table.h"
#include <stdio.h>
#include <stdlib.h>

PageTable* page_table_init(int num_pages) {
    if (num_pages <= 0) return NULL;
    PageTable *pt = (PageTable*)malloc(sizeof(PageTable));
    if (!pt) return NULL;

    pt->num_pages = num_pages;
    pt->entries = (PageTableEntry*)malloc(sizeof(PageTableEntry) * (size_t)num_pages);
    if (!pt->entries) {
        free(pt);
        return NULL;
    }

    for (int i = 0; i < num_pages; i++) {
        pt->entries[i].page_num = i;
        pt->entries[i].frame_num = -1;
        pt->entries[i].valid = false;
    }

    return pt;
}

void page_table_free(PageTable *pt) {
    if (pt) {
        if (pt->entries) {
            free(pt->entries);
        }
        free(pt);
    }
}

void page_table_map(PageTable *pt, int page_num, int frame_num) {
    if (!pt || page_num < 0 || page_num >= pt->num_pages) return;
    pt->entries[page_num].page_num = page_num;
    pt->entries[page_num].frame_num = frame_num;
    pt->entries[page_num].valid = true;
}

void page_table_unmap(PageTable *pt, int page_num) {
    if (!pt || page_num < 0 || page_num >= pt->num_pages) return;
    pt->entries[page_num].frame_num = -1;
    pt->entries[page_num].valid = false;
}

bool page_table_lookup(const PageTable *pt, int page_num, int *frame_out) {
    if (!pt || page_num < 0 || page_num >= pt->num_pages) {
        return false; /* Out of bounds -> Page Fault */
    }

    if (pt->entries[page_num].valid) {
        if (frame_out) {
            *frame_out = pt->entries[page_num].frame_num;
        }
        return true; /* Page Table HIT */
    }

    return false; /* Unmapped page -> Page Fault */
}

void page_table_display(const PageTable *pt) {
    if (!pt) return;
    printf("\n========================================\n");
    printf("              PAGE TABLE                \n");
    printf("========================================\n");
    printf("%-12s %-12s %-12s\n", "Page", "Frame", "Valid");
    printf("----------------------------------------\n");
    for (int i = 0; i < pt->num_pages; i++) {
        if (pt->entries[i].valid) {
            printf("%-12d %-12d %-12s\n", pt->entries[i].page_num, pt->entries[i].frame_num, "YES");
        } else {
            printf("%-12d %-12s %-12s\n", pt->entries[i].page_num, "-", "NO");
        }
    }
    printf("----------------------------------------\n");
}
