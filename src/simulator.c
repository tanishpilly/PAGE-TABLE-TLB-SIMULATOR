#include "simulator.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned int compute_offset_bits(unsigned int page_size) {
    unsigned int bits = 0;
    unsigned int temp = page_size;
    while (temp > 1) {
        temp >>= 1;
        bits++;
    }
    return bits;
}

Simulator* simulator_init(unsigned int page_size, unsigned int num_pages, unsigned int num_frames, unsigned int tlb_size) {
    if (page_size == 0 || (page_size & (page_size - 1)) != 0) {
        fprintf(stderr, "Error: Page size must be a power of 2.\n");
        return NULL;
    }

    Simulator *sim = (Simulator*)malloc(sizeof(Simulator));
    if (!sim) return NULL;

    sim->config.page_size = page_size;
    sim->config.offset_bits = compute_offset_bits(page_size);
    sim->config.page_mask = page_size - 1;
    sim->config.num_pages = num_pages;
    sim->config.num_frames = num_frames;
    sim->config.tlb_size = tlb_size;

    sim->pt = page_table_init((int)num_pages);
    sim->tlb = tlb_init((int)tlb_size);

    if (!sim->pt || !sim->tlb) {
        page_table_free(sim->pt);
        tlb_free(sim->tlb);
        free(sim);
        return NULL;
    }

    sim->stats.total_translations = 0;
    sim->stats.tlb_hits = 0;
    sim->stats.tlb_misses = 0;
    sim->stats.page_table_hits = 0;
    sim->stats.page_faults = 0;

    simulator_load_default_mappings(sim);

    return sim;
}

void simulator_free(Simulator *sim) {
    if (sim) {
        page_table_free(sim->pt);
        tlb_free(sim->tlb);
        free(sim);
    }
}

void simulator_load_default_mappings(Simulator *sim) {
    if (!sim || !sim->pt) return;

    /* Default deterministic mappings */
    page_table_map(sim->pt, 0, 2);
    page_table_map(sim->pt, 1, 5);  /* Page 1 -> Frame 5 (4098 -> Page 1, Offset 2) */
    /* Page 2 is left unmapped (valid = false) */
    page_table_map(sim->pt, 3, 1);
    page_table_map(sim->pt, 4, 7);
    page_table_map(sim->pt, 5, 3);
    /* Page 6 left unmapped */
    page_table_map(sim->pt, 7, 4);
}

TranslationResult simulator_translate(Simulator *sim, unsigned int virtual_address) {
    TranslationResult res;
    memset(&res, 0, sizeof(TranslationResult));

    res.virtual_address = virtual_address;
    res.page_number = virtual_address >> sim->config.offset_bits;
    res.offset = virtual_address & sim->config.page_mask;

    sim->stats.total_translations++;

    /* Bounds check against Virtual Page space */
    if (res.page_number >= sim->config.num_pages) {
        sim->stats.tlb_misses++;
        sim->stats.page_faults++;
        res.tlb_hit = false;
        res.page_table_hit = false;
        res.is_page_fault = true;
        res.frame_number = -1;
        res.physical_address = 0;
        return res;
    }

    /* Step 1: TLB Lookup */
    if (tlb_lookup(sim->tlb, (int)res.page_number, &res.frame_number)) {
        sim->stats.tlb_hits++;
        res.tlb_hit = true;
        res.page_table_hit = true;
        res.is_page_fault = false;
        res.physical_address = ((unsigned int)res.frame_number << sim->config.offset_bits) | res.offset;
        return res;
    }

    /* Step 2: TLB Miss -> Check Page Table */
    sim->stats.tlb_misses++;
    res.tlb_hit = false;

    if (page_table_lookup(sim->pt, (int)res.page_number, &res.frame_number)) {
        sim->stats.page_table_hits++;
        res.page_table_hit = true;
        res.is_page_fault = false;
        res.physical_address = ((unsigned int)res.frame_number << sim->config.offset_bits) | res.offset;

        /* Step 3: Update TLB on Page Table Hit */
        tlb_insert(sim->tlb, (int)res.page_number, res.frame_number);
    } else {
        /* Page Fault */
        sim->stats.page_faults++;
        res.page_table_hit = false;
        res.is_page_fault = true;
        res.frame_number = -1;
        res.physical_address = 0;
    }

    return res;
}

void simulator_print_translation_result(const TranslationResult *res) {
    printf("----------------------------------------\n");
    printf("Address Translation Result\n");
    printf("----------------------------------------\n");
    printf("Virtual Address  : %u\n", res->virtual_address);
    printf("Page Number      : %u\n", res->page_number);
    printf("Offset           : %u\n", res->offset);
    printf("TLB Status       : %s\n", res->tlb_hit ? "HIT" : "MISS");

    if (res->is_page_fault) {
        printf("Page Table       : FAULT\n");
        printf("----------------------------------------\n");
        printf("PAGE FAULT: Unmapped Address\n");
        printf("----------------------------------------\n");
    } else {
        printf("Page Table       : %s\n", res->page_table_hit ? "HIT" : "MISS");
        printf("Frame Number     : %d\n", res->frame_number);
        printf("Physical Address : %u (0x%X)\n", res->physical_address, res->physical_address);
        printf("----------------------------------------\n");
    }
}

void simulator_reset(Simulator *sim) {
    if (!sim) return;
    tlb_flush(sim->tlb);
    sim->stats.total_translations = 0;
    sim->stats.tlb_hits = 0;
    sim->stats.tlb_misses = 0;
    sim->stats.page_table_hits = 0;
    sim->stats.page_faults = 0;
    printf("\nSimulator statistics and TLB cache reset successfully.\n");
}

void simulator_display_stats(const Simulator *sim) {
    if (!sim) return;
    double hit_rate = 0.0;
    if (sim->stats.total_translations > 0) {
        hit_rate = ((double)sim->stats.tlb_hits / (double)sim->stats.total_translations) * 100.0;
    }

    printf("\n========================================\n");
    printf("         SIMULATOR STATISTICS           \n");
    printf("========================================\n");
    printf("Total Address Translations : %lu\n", sim->stats.total_translations);
    printf("TLB Hits                   : %lu\n", sim->stats.tlb_hits);
    printf("TLB Misses                 : %lu\n", sim->stats.tlb_misses);
    printf("Page Table Hits            : %lu\n", sim->stats.page_table_hits);
    printf("Page Faults                : %lu\n", sim->stats.page_faults);
    printf("TLB Hit Rate               : %.2f%%\n", hit_rate);
    printf("========================================\n");
}

int simulator_run_tests(Simulator *sim) {
    if (!sim) return -1;

    printf("\n========================================\n");
    printf("     RUNNING OFFICIAL TEST CASES        \n");
    printf("========================================\n");

    /* Flush TLB and reset stats before test suite */
    tlb_flush(sim->tlb);
    simulator_load_default_mappings(sim);

    int passed = 0;
    int failed = 0;

    /* TEST 1: Address Translation
     * Page Size = 4096, Virtual Address = 4098
     * Expected: Page = 1, Offset = 2
     */
    printf("\n[TEST 1] Address Translation (Virtual Address 4098, Page Size 4096)\n");
    TranslationResult res1 = simulator_translate(sim, 4098);
    if (res1.page_number == 1 && res1.offset == 2) {
        printf("  Page Number : %u (Expected: 1) -> PASS\n", res1.page_number);
        printf("  Offset      : %u (Expected: 2) -> PASS\n", res1.offset);
        printf("  Result      : [ PASS ]\n");
        passed++;
    } else {
        printf("  Page Number : %u (Expected: 1)\n", res1.page_number);
        printf("  Offset      : %u (Expected: 2)\n", res1.offset);
        printf("  Result      : [ FAIL ]\n");
        failed++;
    }

    /* TEST 2: TLB Cache Hit
     * Access address 4098 twice.
     * 1st access -> TLB MISS
     * 2nd access -> TLB HIT
     */
    printf("\n[TEST 2] TLB Cache Hit (Access Virtual Address 4098 twice)\n");
    /* Note: res1 was the first access */
    TranslationResult res2_second = simulator_translate(sim, 4098);
    printf("  First Access TLB Status  : %s (Expected: MISS) -> %s\n", 
           res1.tlb_hit ? "HIT" : "MISS", 
           !res1.tlb_hit ? "PASS" : "FAIL");
    printf("  Second Access TLB Status : %s (Expected: HIT)  -> %s\n", 
           res2_second.tlb_hit ? "HIT" : "MISS", 
           res2_second.tlb_hit ? "PASS" : "FAIL");

    if (!res1.tlb_hit && res2_second.tlb_hit) {
        printf("  Result                   : [ PASS ]\n");
        passed++;
    } else {
        printf("  Result                   : [ FAIL ]\n");
        failed++;
    }

    /* TEST 3: Out-of-Bounds / Unmapped Address
     * Access address outside page table length or unmapped page.
     * Expected: PAGE FAULT: Unmapped Address
     */
    printf("\n[TEST 3] Out-of-Bounds / Unmapped Address Access\n");
    /* Accessing address 65536 (Page 16, which is out of bounds for num_pages = 16) */
    TranslationResult res3_oob = simulator_translate(sim, 65536);
    /* Accessing unmapped page 2 (virtual address 8192) */
    TranslationResult res3_unmapped = simulator_translate(sim, 8192);

    printf("  Address 65536 (Out-of-Bounds Page 16) Status : %s -> %s\n",
           res3_oob.is_page_fault ? "PAGE FAULT: Unmapped Address" : "MAPPED",
           res3_oob.is_page_fault ? "PASS" : "FAIL");
    printf("  Address 8192  (Unmapped Page 2) Status        : %s -> %s\n",
           res3_unmapped.is_page_fault ? "PAGE FAULT: Unmapped Address" : "MAPPED",
           res3_unmapped.is_page_fault ? "PASS" : "FAIL");

    if (res3_oob.is_page_fault && res3_unmapped.is_page_fault) {
        printf("  Result                                        : [ PASS ]\n");
        passed++;
    } else {
        printf("  Result                                        : [ FAIL ]\n");
        failed++;
    }

    printf("\n----------------------------------------\n");
    printf("TEST SUMMARY: %d Passed, %d Failed\n", passed, failed);
    printf("----------------------------------------\n");

    return failed;
}

void simulator_run_demo(Simulator *sim) {
    if (!sim) return;

    printf("\n==================================================\n");
    printf("        DEMONSTRATION MODE FOR FACULTY            \n");
    printf("==================================================\n");

    /* Reset TLB for clean demo */
    tlb_flush(sim->tlb);

    printf("\n1. First Translation of Virtual Address 4098:\n");
    printf("   (Expect: TLB MISS -> Page Table Lookup -> Frame 5 -> TLB Update)\n");
    TranslationResult r1 = simulator_translate(sim, 4098);
    simulator_print_translation_result(&r1);

    printf("\n2. Second Translation of Virtual Address 4098:\n");
    printf("   (Expect: TLB HIT -> Frame 5 directly from TLB)\n");
    TranslationResult r2 = simulator_translate(sim, 4098);
    simulator_print_translation_result(&r2);

    printf("\n3. Translation of Unmapped Virtual Address 8192 (Page 2):\n");
    printf("   (Expect: Page Table Lookup -> PAGE FAULT: Unmapped Address)\n");
    TranslationResult r3 = simulator_translate(sim, 8192);
    simulator_print_translation_result(&r3);

    printf("\n==================================================\n");
    printf("           DEMONSTRATION COMPLETE                 \n");
    printf("==================================================\n");
}
