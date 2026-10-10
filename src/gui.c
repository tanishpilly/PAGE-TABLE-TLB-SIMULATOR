#include "gui.h"
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Structure holding GUI widgets and simulator state */
typedef struct {
    Simulator *sim;
    GtkWidget *window;
    
    /* Input and Result Labels */
    GtkWidget *entry_vaddr;
    GtkWidget *lbl_res_vaddr;
    GtkWidget *lbl_res_page;
    GtkWidget *lbl_res_offset;
    GtkWidget *lbl_res_tlb_status;
    GtkWidget *lbl_res_frame;
    GtkWidget *lbl_res_phys_addr;
    GtkWidget *lbl_res_status;

    /* Flow visualization step labels */
    GtkWidget *lbl_flow_vaddr;
    GtkWidget *lbl_flow_page;
    GtkWidget *lbl_flow_tlb;
    GtkWidget *lbl_flow_pt;
    GtkWidget *lbl_flow_phys;

    /* Tables */
    GtkListStore *store_tlb;
    GtkListStore *store_pt;

    /* Stats Labels */
    GtkWidget *lbl_stat_total;
    GtkWidget *lbl_stat_hits;
    GtkWidget *lbl_stat_misses;
    GtkWidget *lbl_stat_faults;
    GtkWidget *lbl_stat_hit_rate;

    /* Status Bar */
    GtkWidget *lbl_status_msg;

    /* Demo Mode state */
    int demo_step;
} GuiApp;

/* Forward declarations */
static void gui_refresh_all(GuiApp *app);
static void gui_perform_translation(GuiApp *app, unsigned int vaddr);

/* Apply Dark Navy GTK CSS Styling */
static void apply_css(void) {
    GtkCssProvider *provider = gtk_css_provider_new();
    const char *css =
        "window {\n"
        "    background-color: #0B132B;\n"
        "    color: #E2E8F0;\n"
        "    font-family: 'Segoe UI', 'Ubuntu', 'Helvetica Neue', sans-serif;\n"
        "}\n"
        ".card {\n"
        "    background-color: #1C2541;\n"
        "    border: 1px solid #2B3A67;\n"
        "    border-radius: 8px;\n"
        "    padding: 12px;\n"
        "    margin: 6px;\n"
        "}\n"
        ".header-title {\n"
        "    font-size: 20px;\n"
        "    font-weight: bold;\n"
        "    color: #00B4D8;\n"
        "}\n"
        ".header-subtitle {\n"
        "    font-size: 13px;\n"
        "    color: #94A3B8;\n"
        "}\n"
        ".badge-ready {\n"
        "    background-color: #064E3B;\n"
        "    color: #34D399;\n"
        "    border-radius: 12px;\n"
        "    padding: 4px 12px;\n"
        "    font-weight: bold;\n"
        "    font-size: 11px;\n"
        "}\n"
        ".card-title {\n"
        "    font-size: 14px;\n"
        "    font-weight: bold;\n"
        "    color: #48CAE4;\n"
        "    border-bottom: 1px solid #2B3A67;\n"
        "    padding-bottom: 6px;\n"
        "    margin-bottom: 8px;\n"
        "}\n"
        ".btn-primary {\n"
        "    background-color: #0077B6;\n"
        "    color: #FFFFFF;\n"
        "    font-weight: bold;\n"
        "    border-radius: 6px;\n"
        "    border: none;\n"
        "    padding: 6px 16px;\n"
        "}\n"
        ".btn-primary:hover {\n"
        "    background-color: #0096C7;\n"
        "}\n"
        ".btn-action {\n"
        "    background-color: #334155;\n"
        "    color: #F8FAFC;\n"
        "    border-radius: 6px;\n"
        "    border: 1px solid #475569;\n"
        "    padding: 6px 12px;\n"
        "}\n"
        ".btn-action:hover {\n"
        "    background-color: #475569;\n"
        "}\n"
        ".status-hit {\n"
        "    color: #2EC4B6;\n"
        "    font-weight: bold;\n"
        "}\n"
        ".status-miss {\n"
        "    color: #FFB703;\n"
        "    font-weight: bold;\n"
        "}\n"
        ".status-fault {\n"
        "    color: #E63946;\n"
        "    font-weight: bold;\n"
        "}\n"
        ".flow-box {\n"
        "    background-color: #0F172A;\n"
        "    border: 1px solid #334155;\n"
        "    border-radius: 6px;\n"
        "    padding: 8px;\n"
        "}\n"
        "treeview {\n"
        "    background-color: #0F172A;\n"
        "    color: #E2E8F0;\n"
        "}\n"
        "treeview:selected {\n"
        "    background-color: #0077B6;\n"
        "}\n";

    gtk_css_provider_load_from_data(provider, css, -1, NULL);
    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
    );
    g_object_unref(provider);
}

/* Helper to create grid key-value row */
static GtkWidget* add_grid_row(GtkWidget *grid, int row, const char *title) {
    GtkWidget *lbl_t = gtk_label_new(title);
    gtk_label_set_xalign(GTK_LABEL(lbl_t), 0.0);
    GtkWidget *lbl_v = gtk_label_new("-");
    gtk_label_set_xalign(GTK_LABEL(lbl_v), 0.0);

    gtk_grid_attach(GTK_GRID(grid), lbl_t, 0, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), lbl_v, 1, row, 1, 1);
    return lbl_v;
}

/* Helper to update styled label text */
static void set_label(GtkWidget *lbl, const char *text, const char *css_class) {
    if (!lbl) return;
    gtk_label_set_text(GTK_LABEL(lbl), text);
    GtkStyleContext *context = gtk_widget_get_style_context(lbl);
    gtk_style_context_remove_class(context, "status-hit");
    gtk_style_context_remove_class(context, "status-miss");
    gtk_style_context_remove_class(context, "status-fault");
    if (css_class && strlen(css_class) > 0) {
        gtk_style_context_add_class(context, css_class);
    }
}

/* Refresh TLB Table UI */
static void gui_refresh_tlb_table(GuiApp *app) {
    gtk_list_store_clear(app->store_tlb);
    if (!app->sim || !app->sim->tlb) return;

    GtkTreeIter iter;
    for (int i = 0; i < app->sim->tlb->size; i++) {
        TLBEntry entry = app->sim->tlb->entries[i];
        char page_str[32], frame_str[32], used_str[32];

        if (entry.valid) {
            snprintf(page_str, sizeof(page_str), "%d", entry.page_num);
            snprintf(frame_str, sizeof(frame_str), "%d", entry.frame_num);
            snprintf(used_str, sizeof(used_str), "%u", entry.last_used);
        } else {
            snprintf(page_str, sizeof(page_str), "-");
            snprintf(frame_str, sizeof(frame_str), "-");
            snprintf(used_str, sizeof(used_str), "-");
        }

        gtk_list_store_append(app->store_tlb, &iter);
        gtk_list_store_set(app->store_tlb, &iter,
                           0, i,
                           1, page_str,
                           2, frame_str,
                           3, entry.valid ? "VALID" : "EMPTY",
                           4, used_str,
                           -1);
    }
}

/* Refresh Page Table UI */
static void gui_refresh_pt_table(GuiApp *app) {
    gtk_list_store_clear(app->store_pt);
    if (!app->sim || !app->sim->pt) return;

    GtkTreeIter iter;
    for (int i = 0; i < app->sim->pt->num_pages; i++) {
        PageTableEntry entry = app->sim->pt->entries[i];
        char frame_str[32];

        if (entry.valid) {
            snprintf(frame_str, sizeof(frame_str), "%d", entry.frame_num);
        } else {
            snprintf(frame_str, sizeof(frame_str), "-");
        }

        gtk_list_store_append(app->store_pt, &iter);
        gtk_list_store_set(app->store_pt, &iter,
                           0, entry.page_num,
                           1, frame_str,
                           2, entry.valid ? "YES" : "NO",
                           -1);
    }
}

/* Refresh Live Statistics UI */
static void gui_refresh_stats(GuiApp *app) {
    if (!app->sim) return;

    char buf[64];
    snprintf(buf, sizeof(buf), "%lu", app->sim->stats.total_translations);
    set_label(app->lbl_stat_total, buf, "");

    snprintf(buf, sizeof(buf), "%lu", app->sim->stats.tlb_hits);
    set_label(app->lbl_stat_hits, buf, "status-hit");

    snprintf(buf, sizeof(buf), "%lu", app->sim->stats.tlb_misses);
    set_label(app->lbl_stat_misses, buf, "status-miss");

    snprintf(buf, sizeof(buf), "%lu", app->sim->stats.page_faults);
    set_label(app->lbl_stat_faults, buf, "status-fault");

    double hit_rate = 0.0;
    if (app->sim->stats.total_translations > 0) {
        hit_rate = ((double)app->sim->stats.tlb_hits / (double)app->sim->stats.total_translations) * 100.0;
    }
    snprintf(buf, sizeof(buf), "%.2f%%", hit_rate);
    set_label(app->lbl_stat_hit_rate, buf, "status-hit");
}

/* Refresh all panels */
static void gui_refresh_all(GuiApp *app) {
    gui_refresh_tlb_table(app);
    gui_refresh_pt_table(app);
    gui_refresh_stats(app);
}

/* Perform Virtual Address Translation and Update UI */
static void gui_perform_translation(GuiApp *app, unsigned int vaddr) {
    TranslationResult res = simulator_translate(app->sim, vaddr);

    char buf[64];
    snprintf(buf, sizeof(buf), "%u", res.virtual_address);
    set_label(app->lbl_res_vaddr, buf, "");

    snprintf(buf, sizeof(buf), "%u", res.page_number);
    set_label(app->lbl_res_page, buf, "");

    snprintf(buf, sizeof(buf), "%u", res.offset);
    set_label(app->lbl_res_offset, buf, "");

    set_label(app->lbl_res_tlb_status, res.tlb_hit ? "HIT" : "MISS", res.tlb_hit ? "status-hit" : "status-miss");

    if (res.is_page_fault) {
        set_label(app->lbl_res_frame, "-", "");
        set_label(app->lbl_res_phys_addr, "-", "");
        set_label(app->lbl_res_status, "PAGE FAULT: Unmapped Address", "status-fault");

        /* Update Flow Visualization for Page Fault */
        snprintf(buf, sizeof(buf), "1. Virtual Addr: %u", vaddr);
        set_label(app->lbl_flow_vaddr, buf, "");
        snprintf(buf, sizeof(buf), "2. Page: %u, Offset: %u", res.page_number, res.offset);
        set_label(app->lbl_flow_page, buf, "");
        set_label(app->lbl_flow_tlb, "3. TLB Search: MISS", "status-miss");
        set_label(app->lbl_flow_pt, "4. Page Table: UNMAPPED / OUT OF BOUNDS", "status-fault");
        set_label(app->lbl_flow_phys, "5. Result: PAGE FAULT TRIGGERED", "status-fault");
        gtk_label_set_text(GTK_LABEL(app->lbl_status_msg), "Status: Page Fault detected for unmapped address!");
    } else {
        snprintf(buf, sizeof(buf), "%d", res.frame_number);
        set_label(app->lbl_res_frame, buf, "");

        snprintf(buf, sizeof(buf), "%u (0x%X)", res.physical_address, res.physical_address);
        set_label(app->lbl_res_phys_addr, buf, "");

        set_label(app->lbl_res_status, "SUCCESS", "status-hit");

        /* Update Flow Visualization */
        snprintf(buf, sizeof(buf), "1. Virtual Addr: %u", vaddr);
        set_label(app->lbl_flow_vaddr, buf, "");
        snprintf(buf, sizeof(buf), "2. Page: %u, Offset: %u", res.page_number, res.offset);
        set_label(app->lbl_flow_page, buf, "");

        if (res.tlb_hit) {
            set_label(app->lbl_flow_tlb, "3. TLB Search: HIT (Frame found in TLB)", "status-hit");
            set_label(app->lbl_flow_pt, "4. Page Table: BYPASSED (TLB Hit)", "status-hit");
        } else {
            set_label(app->lbl_flow_tlb, "3. TLB Search: MISS", "status-miss");
            snprintf(buf, sizeof(buf), "4. Page Table: HIT (Page %u -> Frame %d) => Updated TLB", res.page_number, res.frame_number);
            set_label(app->lbl_flow_pt, buf, "status-hit");
        }

        snprintf(buf, sizeof(buf), "5. Phys Addr: (Frame %d << 12) | %u = %u", res.frame_number, res.offset, res.physical_address);
        set_label(app->lbl_flow_phys, buf, "status-hit");

        gtk_label_set_text(GTK_LABEL(app->lbl_status_msg), res.tlb_hit ? "Status: Translation successful (TLB HIT)" : "Status: Translation successful (TLB MISS -> Page Table HIT)");
    }

    gui_refresh_all(app);
}

/* Callback for Translate button */
static void on_btn_translate_clicked(GtkWidget *widget, gpointer data) {
    (void)widget;
    GuiApp *app = (GuiApp*)data;
    const char *text = gtk_entry_get_text(GTK_ENTRY(app->entry_vaddr));

    if (!text || strlen(text) == 0) {
        gtk_label_set_text(GTK_LABEL(app->lbl_status_msg), "Error: Virtual address field is empty!");
        return;
    }

    char *endptr;
    long vaddr = strtol(text, &endptr, 10);
    if (*endptr != '\0' || vaddr < 0) {
        gtk_label_set_text(GTK_LABEL(app->lbl_status_msg), "Error: Invalid non-negative decimal virtual address!");
        return;
    }

    gui_perform_translation(app, (unsigned int)vaddr);
}

/* Callback for Reset button */
static void on_btn_reset_clicked(GtkWidget *widget, gpointer data) {
    (void)widget;
    GuiApp *app = (GuiApp*)data;
    simulator_reset(app->sim);
    gui_refresh_all(app);
    gtk_label_set_text(GTK_LABEL(app->lbl_status_msg), "Status: TLB and statistics reset cleanly.");
}

/* Callback for Clear TLB button */
static void on_btn_clear_tlb_clicked(GtkWidget *widget, gpointer data) {
    (void)widget;
    GuiApp *app = (GuiApp*)data;
    tlb_flush(app->sim->tlb);
    gui_refresh_all(app);
    gtk_label_set_text(GTK_LABEL(app->lbl_status_msg), "Status: TLB flushed successfully.");
}

/* Callback for Refresh Page Table button */
static void on_btn_refresh_pt_clicked(GtkWidget *widget, gpointer data) {
    (void)widget;
    GuiApp *app = (GuiApp*)data;
    gui_refresh_pt_table(app);
    gtk_label_set_text(GTK_LABEL(app->lbl_status_msg), "Status: Page table display refreshed.");
}

/* Run Official Automated Test Suite Dialog */
static void on_btn_run_tests_clicked(GtkWidget *widget, gpointer data) {
    (void)widget;
    GuiApp *app = (GuiApp*)data;

    GtkWidget *dialog = gtk_dialog_new_with_buttons("Official Test Cases",
                                                     GTK_WINDOW(app->window),
                                                     GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                                     "Close", GTK_RESPONSE_CLOSE,
                                                     NULL);
    gtk_window_set_default_size(GTK_WINDOW(dialog), 650, 400);

    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 15);
    gtk_container_add(GTK_CONTAINER(content_area), vbox);

    GtkWidget *lbl_title = gtk_label_new("OFFICIAL SIMULATOR TEST SUITE");
    gtk_widget_set_name(lbl_title, "header-title");
    gtk_box_pack_start(GTK_BOX(vbox), lbl_title, FALSE, FALSE, 0);

    /* Run backend tests */
    tlb_flush(app->sim->tlb);
    simulator_load_default_mappings(app->sim);

    /* Test 1 */
    TranslationResult r1 = simulator_translate(app->sim, 4098);
    bool t1_pass = (r1.page_number == 1 && r1.offset == 2);

    /* Test 2 */
    TranslationResult r2 = simulator_translate(app->sim, 4098);
    bool t2_pass = (!r1.tlb_hit && r2.tlb_hit);

    /* Test 3 */
    TranslationResult r3_oob = simulator_translate(app->sim, 65536);
    TranslationResult r3_unmap = simulator_translate(app->sim, 8192);
    bool t3_pass = (r3_oob.is_page_fault && r3_unmap.is_page_fault);

    char buf[256];
    snprintf(buf, sizeof(buf), "TEST 1: Address Translation (Virtual 4098, Page Size 4096)\n  Page: %u (Exp: 1), Offset: %u (Exp: 2) -> Status: %s",
             r1.page_number, r1.offset, t1_pass ? "PASS" : "FAIL");
    GtkWidget *lbl_t1 = gtk_label_new(buf);
    gtk_label_set_xalign(GTK_LABEL(lbl_t1), 0.0);
    gtk_box_pack_start(GTK_BOX(vbox), lbl_t1, FALSE, FALSE, 5);

    snprintf(buf, sizeof(buf), "TEST 2: TLB Cache Hit (Access 4098 Twice)\n  1st Access: %s (Exp: MISS), 2nd Access: %s (Exp: HIT) -> Status: %s",
             r1.tlb_hit ? "HIT" : "MISS", r2.tlb_hit ? "HIT" : "MISS", t2_pass ? "PASS" : "FAIL");
    GtkWidget *lbl_t2 = gtk_label_new(buf);
    gtk_label_set_xalign(GTK_LABEL(lbl_t2), 0.0);
    gtk_box_pack_start(GTK_BOX(vbox), lbl_t2, FALSE, FALSE, 5);

    snprintf(buf, sizeof(buf), "TEST 3: Out-of-Bounds & Unmapped Page Access\n  Addr 65536 (OOB): %s, Addr 8192 (Unmapped): %s -> Status: %s",
             r3_oob.is_page_fault ? "FAULT" : "OK", r3_unmap.is_page_fault ? "FAULT" : "OK", t3_pass ? "PASS" : "FAIL");
    GtkWidget *lbl_t3 = gtk_label_new(buf);
    gtk_label_set_xalign(GTK_LABEL(lbl_t3), 0.0);
    gtk_box_pack_start(GTK_BOX(vbox), lbl_t3, FALSE, FALSE, 5);

    snprintf(buf, sizeof(buf), "OVERALL RESULT: %d / 3 TEST SUITES PASSED", (t1_pass?1:0) + (t2_pass?1:0) + (t3_pass?1:0));
    GtkWidget *lbl_sum = gtk_label_new(buf);
    gtk_box_pack_start(GTK_BOX(vbox), lbl_sum, FALSE, FALSE, 10);

    gui_refresh_all(app);
    gtk_widget_show_all(dialog);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

/* Guided Demonstration Mode */
static void on_btn_demo_clicked(GtkWidget *widget, gpointer data) {
    (void)widget;
    GuiApp *app = (GuiApp*)data;

    app->demo_step++;
    if (app->demo_step > 3) {
        app->demo_step = 1;
        tlb_flush(app->sim->tlb);
    }

    if (app->demo_step == 1) {
        gtk_entry_set_text(GTK_ENTRY(app->entry_vaddr), "4098");
        gui_perform_translation(app, 4098);
        gtk_label_set_text(GTK_LABEL(app->lbl_status_msg), "DEMO STEP 1: First access of 4098 yields TLB MISS -> Page Table HIT (Frame 5).");
    } else if (app->demo_step == 2) {
        gtk_entry_set_text(GTK_ENTRY(app->entry_vaddr), "4098");
        gui_perform_translation(app, 4098);
        gtk_label_set_text(GTK_LABEL(app->lbl_status_msg), "DEMO STEP 2: Second access of 4098 yields TLB HIT directly!");
    } else if (app->demo_step == 3) {
        gtk_entry_set_text(GTK_ENTRY(app->entry_vaddr), "8192");
        gui_perform_translation(app, 8192);
        gtk_label_set_text(GTK_LABEL(app->lbl_status_msg), "DEMO STEP 3: Accessing unmapped address 8192 yields PAGE FAULT.");
    }
}

/* Build Dashboard Window */
int gui_run(int argc, char *argv[], Simulator *sim) {
    gtk_init(&argc, &argv);
    apply_css();

    GuiApp app;
    memset(&app, 0, sizeof(GuiApp));
    app.sim = sim;
    app.demo_step = 0;

    /* Main Window */
    app.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(app.window), "PAGE TABLE & TLB SIMULATOR");
    gtk_window_set_default_size(GTK_WINDOW(app.window), 1150, 780);
    g_signal_connect(app.window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *root_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(app.window), root_vbox);

    /* ---------------- HEADER BAR ---------------- */
    GtkWidget *header_card = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    GtkStyleContext *header_ctx = gtk_widget_get_style_context(header_card);
    gtk_style_context_add_class(header_ctx, "card");

    GtkWidget *header_title_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    GtkWidget *lbl_title = gtk_label_new("PAGE TABLE & TLB SIMULATOR");
    GtkStyleContext *t_ctx = gtk_widget_get_style_context(lbl_title);
    gtk_style_context_add_class(t_ctx, "header-title");
    gtk_label_set_xalign(GTK_LABEL(lbl_title), 0.0);

    GtkWidget *lbl_sub = gtk_label_new("Virtual Memory Address Translation Dashboard");
    GtkStyleContext *s_ctx = gtk_widget_get_style_context(lbl_sub);
    gtk_style_context_add_class(s_ctx, "header-subtitle");
    gtk_label_set_xalign(GTK_LABEL(lbl_sub), 0.0);

    gtk_box_pack_start(GTK_BOX(header_title_box), lbl_title, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(header_title_box), lbl_sub, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(header_card), header_title_box, TRUE, TRUE, 0);

    GtkWidget *badge_ready = gtk_label_new("● SIMULATOR READY");
    GtkStyleContext *b_ctx = gtk_widget_get_style_context(badge_ready);
    gtk_style_context_add_class(b_ctx, "badge-ready");
    gtk_box_pack_end(GTK_BOX(header_card), badge_ready, FALSE, FALSE, 5);

    gtk_box_pack_start(GTK_BOX(root_vbox), header_card, FALSE, FALSE, 0);

    /* ---------------- MAIN BODY (SPLIT PANES) ---------------- */
    GtkWidget *body_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(root_vbox), body_hbox, TRUE, TRUE, 0);

    /* ---------- LEFT COLUMN (Translate & Flow) ---------- */
    GtkWidget *left_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start(GTK_BOX(body_hbox), left_vbox, TRUE, TRUE, 0);

    /* CARD 1: TRANSLATE VIRTUAL ADDRESS */
    GtkWidget *card_translate = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_style_context_add_class(gtk_widget_get_style_context(card_translate), "card");

    GtkWidget *lbl_c1 = gtk_label_new("1. TRANSLATE VIRTUAL ADDRESS");
    gtk_style_context_add_class(gtk_widget_get_style_context(lbl_c1), "card-title");
    gtk_label_set_xalign(GTK_LABEL(lbl_c1), 0.0);
    gtk_box_pack_start(GTK_BOX(card_translate), lbl_c1, FALSE, FALSE, 0);

    GtkWidget *input_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    GtkWidget *lbl_in_prompt = gtk_label_new("Virtual Address:");
    app.entry_vaddr = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(app.entry_vaddr), "e.g. 4098");
    GtkWidget *btn_translate = gtk_button_new_with_label("TRANSLATE");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_translate), "btn-primary");
    g_signal_connect(btn_translate, "clicked", G_CALLBACK(on_btn_translate_clicked), &app);

    gtk_box_pack_start(GTK_BOX(input_hbox), lbl_in_prompt, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(input_hbox), app.entry_vaddr, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(input_hbox), btn_translate, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(card_translate), input_hbox, FALSE, FALSE, 4);

    /* Translation Results Grid */
    GtkWidget *res_grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(res_grid), 6);
    gtk_grid_set_column_spacing(GTK_GRID(res_grid), 20);

    app.lbl_res_vaddr      = add_grid_row(res_grid, 0, "Virtual Address:");
    app.lbl_res_page       = add_grid_row(res_grid, 1, "Page Number:");
    app.lbl_res_offset     = add_grid_row(res_grid, 2, "Offset:");
    app.lbl_res_tlb_status = add_grid_row(res_grid, 3, "TLB Status:");
    app.lbl_res_frame      = add_grid_row(res_grid, 4, "Frame Number:");
    app.lbl_res_phys_addr  = add_grid_row(res_grid, 5, "Physical Address:");
    app.lbl_res_status     = add_grid_row(res_grid, 6, "Status:");

    gtk_box_pack_start(GTK_BOX(card_translate), res_grid, FALSE, FALSE, 4);
    gtk_box_pack_start(GTK_BOX(left_vbox), card_translate, FALSE, FALSE, 0);

    /* CARD 2: TRANSLATION FLOW VISUALIZATION */
    GtkWidget *card_flow = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_style_context_add_class(gtk_widget_get_style_context(card_flow), "card");

    GtkWidget *lbl_c2 = gtk_label_new("2. TRANSLATION FLOW VISUALIZATION");
    gtk_style_context_add_class(gtk_widget_get_style_context(lbl_c2), "card-title");
    gtk_label_set_xalign(GTK_LABEL(lbl_c2), 0.0);
    gtk_box_pack_start(GTK_BOX(card_flow), lbl_c2, FALSE, FALSE, 0);

    GtkWidget *flow_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_style_context_add_class(gtk_widget_get_style_context(flow_vbox), "flow-box");

    app.lbl_flow_vaddr = gtk_label_new("1. Virtual Addr: (Awaiting Translation)");
    gtk_label_set_xalign(GTK_LABEL(app.lbl_flow_vaddr), 0.0);
    app.lbl_flow_page  = gtk_label_new("2. Page + Offset: -");
    gtk_label_set_xalign(GTK_LABEL(app.lbl_flow_page), 0.0);
    app.lbl_flow_tlb   = gtk_label_new("3. TLB Search: -");
    gtk_label_set_xalign(GTK_LABEL(app.lbl_flow_tlb), 0.0);
    app.lbl_flow_pt    = gtk_label_new("4. Page Table: -");
    gtk_label_set_xalign(GTK_LABEL(app.lbl_flow_pt), 0.0);
    app.lbl_flow_phys  = gtk_label_new("5. Physical Address: -");
    gtk_label_set_xalign(GTK_LABEL(app.lbl_flow_phys), 0.0);

    gtk_box_pack_start(GTK_BOX(flow_vbox), app.lbl_flow_vaddr, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(flow_vbox), app.lbl_flow_page, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(flow_vbox), app.lbl_flow_tlb, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(flow_vbox), app.lbl_flow_pt, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(flow_vbox), app.lbl_flow_phys, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(card_flow), flow_vbox, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(left_vbox), card_flow, TRUE, TRUE, 0);

    /* ---------- RIGHT COLUMN (TLB, Page Table, Stats) ---------- */
    GtkWidget *right_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start(GTK_BOX(body_hbox), right_vbox, TRUE, TRUE, 0);

    /* CARD 3: TLB CACHE PANEL */
    GtkWidget *card_tlb = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_style_context_add_class(gtk_widget_get_style_context(card_tlb), "card");

    GtkWidget *tlb_hdr = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    GtkWidget *lbl_c3 = gtk_label_new("3. TLB CACHE");
    gtk_style_context_add_class(gtk_widget_get_style_context(lbl_c3), "card-title");
    gtk_label_set_xalign(GTK_LABEL(lbl_c3), 0.0);
    GtkWidget *btn_clear_tlb = gtk_button_new_with_label("CLEAR TLB");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_clear_tlb), "btn-action");
    g_signal_connect(btn_clear_tlb, "clicked", G_CALLBACK(on_btn_clear_tlb_clicked), &app);

    gtk_box_pack_start(GTK_BOX(tlb_hdr), lbl_c3, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(tlb_hdr), btn_clear_tlb, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(card_tlb), tlb_hdr, FALSE, FALSE, 0);

    /* TLB List Store */
    app.store_tlb = gtk_list_store_new(5, G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
    GtkWidget *tree_tlb = gtk_tree_view_new_with_model(GTK_TREE_MODEL(app.store_tlb));

    const char *col_tlb[] = {"Entry", "Page", "Frame", "Status", "Last Used"};
    for (int i = 0; i < 5; i++) {
        GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
        GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes(col_tlb[i], renderer, "text", i, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(tree_tlb), column);
    }

    GtkWidget *scroll_tlb = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(scroll_tlb), 120);
    gtk_container_add(GTK_CONTAINER(scroll_tlb), tree_tlb);
    gtk_box_pack_start(GTK_BOX(card_tlb), scroll_tlb, TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(right_vbox), card_tlb, TRUE, TRUE, 0);

    /* CARD 4: PAGE TABLE PANEL */
    GtkWidget *card_pt = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_style_context_add_class(gtk_widget_get_style_context(card_pt), "card");

    GtkWidget *pt_hdr = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    GtkWidget *lbl_c4 = gtk_label_new("4. PAGE TABLE");
    gtk_style_context_add_class(gtk_widget_get_style_context(lbl_c4), "card-title");
    gtk_label_set_xalign(GTK_LABEL(lbl_c4), 0.0);
    GtkWidget *btn_ref_pt = gtk_button_new_with_label("REFRESH");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_ref_pt), "btn-action");
    g_signal_connect(btn_ref_pt, "clicked", G_CALLBACK(on_btn_refresh_pt_clicked), &app);

    gtk_box_pack_start(GTK_BOX(pt_hdr), lbl_c4, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(pt_hdr), btn_ref_pt, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(card_pt), pt_hdr, FALSE, FALSE, 0);

    app.store_pt = gtk_list_store_new(3, G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING);
    GtkWidget *tree_pt = gtk_tree_view_new_with_model(GTK_TREE_MODEL(app.store_pt));

    const char *col_pt[] = {"Page", "Frame", "Valid"};
    for (int i = 0; i < 3; i++) {
        GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
        GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes(col_pt[i], renderer, "text", i, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(tree_pt), column);
    }

    GtkWidget *scroll_pt = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(scroll_pt), 140);
    gtk_container_add(GTK_CONTAINER(scroll_pt), tree_pt);
    gtk_box_pack_start(GTK_BOX(card_pt), scroll_pt, TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(right_vbox), card_pt, TRUE, TRUE, 0);

    /* CARD 5: LIVE STATISTICS PANEL */
    GtkWidget *card_stats = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_style_context_add_class(gtk_widget_get_style_context(card_stats), "card");

    GtkWidget *lbl_c5 = gtk_label_new("5. SIMULATOR STATISTICS");
    gtk_style_context_add_class(gtk_widget_get_style_context(lbl_c5), "card-title");
    gtk_label_set_xalign(GTK_LABEL(lbl_c5), 0.0);
    gtk_box_pack_start(GTK_BOX(card_stats), lbl_c5, FALSE, FALSE, 0);

    GtkWidget *s_grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(s_grid), 4);
    gtk_grid_set_column_spacing(GTK_GRID(s_grid), 16);

    app.lbl_stat_total    = add_grid_row(s_grid, 0, "Total Translations:");
    app.lbl_stat_hits     = add_grid_row(s_grid, 1, "TLB Hits:");
    app.lbl_stat_misses   = add_grid_row(s_grid, 2, "TLB Misses:");
    app.lbl_stat_faults   = add_grid_row(s_grid, 3, "Page Faults:");
    app.lbl_stat_hit_rate = add_grid_row(s_grid, 4, "TLB Hit Rate:");

    gtk_box_pack_start(GTK_BOX(card_stats), s_grid, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(right_vbox), card_stats, FALSE, FALSE, 0);

    /* ---------------- BOTTOM ACTION BAR ---------------- */
    GtkWidget *action_card = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_style_context_add_class(gtk_widget_get_style_context(action_card), "card");

    GtkWidget *btn_run_tests = gtk_button_new_with_label("RUN TESTS");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_run_tests), "btn-action");
    g_signal_connect(btn_run_tests, "clicked", G_CALLBACK(on_btn_run_tests_clicked), &app);

    GtkWidget *btn_demo = gtk_button_new_with_label("DEMO MODE");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_demo), "btn-action");
    g_signal_connect(btn_demo, "clicked", G_CALLBACK(on_btn_demo_clicked), &app);

    GtkWidget *btn_reset = gtk_button_new_with_label("RESET");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_reset), "btn-action");
    g_signal_connect(btn_reset, "clicked", G_CALLBACK(on_btn_reset_clicked), &app);

    GtkWidget *btn_exit = gtk_button_new_with_label("EXIT");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_exit), "btn-action");
    g_signal_connect(btn_exit, "clicked", G_CALLBACK(gtk_main_quit), NULL);

    app.lbl_status_msg = gtk_label_new("Status: Simulator loaded and ready for queries.");
    gtk_label_set_xalign(GTK_LABEL(app.lbl_status_msg), 0.0);

    gtk_box_pack_start(GTK_BOX(action_card), app.lbl_status_msg, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(action_card), btn_run_tests, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(action_card), btn_demo, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(action_card), btn_reset, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(action_card), btn_exit, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(root_vbox), action_card, FALSE, FALSE, 0);

    /* Initial Refresh */
    gui_refresh_all(&app);

    gtk_widget_show_all(app.window);
    gtk_main();

    return EXIT_SUCCESS;
}
