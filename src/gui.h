#ifndef GUI_H
#define GUI_H

#include "simulator.h"

/**
 * Launch the GTK Graphical User Interface dashboard for the Page Table & TLB Simulator.
 * Takes command line arguments and pointer to the shared Simulator instance.
 * Returns EXIT_SUCCESS or EXIT_FAILURE.
 */
int gui_run(int argc, char *argv[], Simulator *sim);

#endif /* GUI_H */
