#ifndef PAGE_TABLE_H
#define PAGE_TABLE_H

#include <stdbool.h>

/**
 * Structure representing a single Page Table Entry (PTE).
 */
typedef struct {
    int page_num;   /* Virtual page number */
    int frame_num;  /* Physical frame number (-1 if invalid) */
    bool valid;     /* Valid flag (true = mapped, false = unmapped) */
} PageTableEntry;

/**
 * Structure representing the Page Table.
 */
typedef struct {
    PageTableEntry *entries;
    int num_pages;  /* Total number of virtual pages supported */
} PageTable;

/**
 * Initialize a Page Table with the specified number of virtual pages.
 * All entries are initially marked as invalid.
 */
PageTable* page_table_init(int num_pages);

/**
 * Free resources allocated for the Page Table.
 */
void page_table_free(PageTable *pt);

/**
 * Map a virtual page number to a physical frame number.
 */
void page_table_map(PageTable *pt, int page_num, int frame_num);

/**
 * Unmap a virtual page number.
 */
void page_table_unmap(PageTable *pt, int page_num);

/**
 * Lookup a virtual page number in the Page Table.
 * Returns true if valid mapping found (Page Table HIT), false on Page Fault.
 * If true, stores physical frame number in *frame_out.
 */
bool page_table_lookup(const PageTable *pt, int page_num, int *frame_out);

/**
 * Display the current status of the Page Table.
 */
void page_table_display(const PageTable *pt);

#endif /* PAGE_TABLE_H */
