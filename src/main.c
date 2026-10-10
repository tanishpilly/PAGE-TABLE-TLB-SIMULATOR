#include "simulator.h"
#ifdef ENABLE_GUI
#include "gui.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_menu(void) {
    printf("\n========================================\n");
    printf("     PAGE TABLE & TLB SIMULATOR         \n");
    printf("========================================\n");
    printf("1. Translate Virtual Address\n");
    printf("2. View Page Table\n");
    printf("3. View TLB\n");
    printf("4. Run Test Cases\n");
    printf("5. View Statistics\n");
    printf("6. Demonstration Mode\n");
    printf("7. Reset Simulator\n");
    printf("8. Exit\n");
    printf("========================================\n");
    printf("Select an option (1-8): ");
}

static void read_input_line(char *buffer, size_t size) {
    if (fgets(buffer, (int)size, stdin) == NULL) {
        buffer[0] = '\0';
    } else {
        buffer[strcspn(buffer, "\r\n")] = '\0';
    }
}

int main(int argc, char *argv[]) {
    /* Sensible Default Configuration */
    const unsigned int DEFAULT_PAGE_SIZE = 4096;
    const unsigned int DEFAULT_NUM_PAGES = 16;
    const unsigned int DEFAULT_NUM_FRAMES = 8;
    const unsigned int DEFAULT_TLB_SIZE = 4;

    Simulator *sim = simulator_init(DEFAULT_PAGE_SIZE, DEFAULT_NUM_PAGES, DEFAULT_NUM_FRAMES, DEFAULT_TLB_SIZE);
    if (!sim) {
        fprintf(stderr, "Failed to initialize simulator.\n");
        return EXIT_FAILURE;
    }

    /* Check if executable name or flag requests GUI mode */
    bool launch_gui = false;
    if (argv[0] && strstr(argv[0], "gui") != NULL) {
        launch_gui = true;
    }
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--gui") == 0 || strcmp(argv[i], "-g") == 0) {
            launch_gui = true;
            break;
        }
    }

    if (launch_gui) {
#ifdef ENABLE_GUI
        int ret = gui_run(argc, argv, sim);
        simulator_free(sim);
        return ret;
#else
        printf("GUI mode is not enabled in this CLI executable. Please launch ./pagetable_tlb_gui.\n");
        simulator_free(sim);
        return EXIT_FAILURE;
#endif
    }

    /* Check CLI command line flags */
    if (argc > 1) {
        if (strcmp(argv[1], "--test") == 0 || strcmp(argv[1], "-t") == 0) {
            int ret = simulator_run_tests(sim);
            simulator_free(sim);
            return (ret == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
        } else if (strcmp(argv[1], "--demo") == 0 || strcmp(argv[1], "-d") == 0) {
            simulator_run_demo(sim);
            simulator_free(sim);
            return EXIT_SUCCESS;
        } else if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
            printf("Usage: %s [OPTION]\n", argv[0]);
            printf("Options:\n");
            printf("  --gui,  -g    Launch GTK Graphical User Interface\n");
            printf("  --test, -t    Run automated test suite in CLI mode\n");
            printf("  --demo, -d    Run faculty demonstration mode in CLI mode\n");
            printf("  --help, -h    Display this help message\n");
            simulator_free(sim);
            return EXIT_SUCCESS;
        }
    }

    /* Interactive Terminal Menu Mode */
    char input_buf[128];
    int choice = 0;

    while (1) {
        print_menu();
        read_input_line(input_buf, sizeof(input_buf));

        if (sscanf(input_buf, "%d", &choice) != 1) {
            printf("\nInvalid input! Please enter a number between 1 and 8.\n");
            continue;
        }

        switch (choice) {
            case 1: {
                printf("\nEnter Virtual Address: ");
                read_input_line(input_buf, sizeof(input_buf));
                unsigned int vaddr = 0;
                if (sscanf(input_buf, "%u", &vaddr) != 1) {
                    printf("\nInvalid virtual address input!\n");
                } else {
                    TranslationResult res = simulator_translate(sim, vaddr);
                    simulator_print_translation_result(&res);
                }
                break;
            }
            case 2:
                page_table_display(sim->pt);
                break;
            case 3:
                tlb_display(sim->tlb);
                break;
            case 4:
                simulator_run_tests(sim);
                break;
            case 5:
                simulator_display_stats(sim);
                break;
            case 6:
                simulator_run_demo(sim);
                break;
            case 7:
                simulator_reset(sim);
                break;
            case 8:
                printf("\nExiting Page Table & TLB Simulator. Goodbye!\n");
                simulator_free(sim);
                return EXIT_SUCCESS;
            default:
                printf("\nInvalid option! Please select a valid menu choice (1-8).\n");
                break;
        }
    }

    simulator_free(sim);
    return EXIT_SUCCESS;
}
