#ifndef SIMULATOR_H
#define SIMULATOR_H

#include "page_table.h"
#include "tlb.h"
#include <stdbool.h>

/**
 * Simulator configuration settings.
 */
typedef struct {
    unsigned int page_size;    /* Size of page in bytes (e.g. 4096) */
    unsigned int offset_bits;  /* Number of bits for page offset (e.g. 12) */
    unsigned int page_mask;    /* Mask to extract offset (page_size - 1) */
    unsigned int num_pages;    /* Total virtual pages in Page Table */
    unsigned int num_frames;   /* Total physical frames */
    unsigned int tlb_size;     /* Total entries in TLB */
} SimConfig;

/**
 * Simulator statistics.
 */
typedef struct {
    unsigned long total_translations;
    unsigned long tlb_hits;
    unsigned long tlb_misses;
    unsigned long page_table_hits;
    unsigned long page_faults;
} SimStats;

/**
 * Address translation result container.
 */
typedef struct {
    unsigned int virtual_address;
    unsigned int page_number;
    unsigned int offset;
    bool tlb_hit;
    bool page_table_hit;
    int frame_number;
    unsigned int physical_address;
    bool is_page_fault;
} TranslationResult;

/**
 * Main simulator container holding configuration, page table, TLB, and stats.
 */
typedef struct {
    SimConfig config;
    PageTable *pt;
    TLB *tlb;
    SimStats stats;
} Simulator;

/**
 * Initialize simulator with page size, total virtual pages, physical frames, and TLB size.
 */
Simulator* simulator_init(unsigned int page_size, unsigned int num_pages, unsigned int num_frames, unsigned int tlb_size);

/**
 * Free simulator resources.
 */
void simulator_free(Simulator *sim);

/**
 * Populate default page table mapping for demonstration.
 */
void simulator_load_default_mappings(Simulator *sim);

/**
 * Perform address translation for given virtual address.
 */
TranslationResult simulator_translate(Simulator *sim, unsigned int virtual_address);

/**
 * Print formatted translation result.
 */
void simulator_print_translation_result(const TranslationResult *res);

/**
 * Reset statistics and clear TLB.
 */
void simulator_reset(Simulator *sim);

/**
 * Display simulator statistics.
 */
void simulator_display_stats(const Simulator *sim);

/**
 * Run official automated test suite. Returns 0 on success, non-zero on failure.
 */
int simulator_run_tests(Simulator *sim);

/**
 * Run guided demonstration mode for faculty evaluation.
 */
void simulator_run_demo(Simulator *sim);

#endif /* SIMULATOR_H */
