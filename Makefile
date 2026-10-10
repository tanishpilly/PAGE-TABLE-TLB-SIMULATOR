CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -pedantic -Isrc
GTK_CFLAGS = $(shell pkg-config --cflags gtk+-3.0 2>/dev/null)
GTK_LIBS = $(shell pkg-config --libs gtk+-3.0 2>/dev/null)

CLI_TARGET = pagetable_tlb_sim
GUI_TARGET = pagetable_tlb_gui

BUILD_DIR = build
SRC_DIR = src

COMMON_SRCS = $(SRC_DIR)/page_table.c $(SRC_DIR)/tlb.c $(SRC_DIR)/simulator.c
COMMON_OBJS = $(COMMON_SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

CLI_OBJS = $(BUILD_DIR)/main_cli.o $(COMMON_OBJS)
GUI_OBJS = $(BUILD_DIR)/main_gui.o $(BUILD_DIR)/gui.o $(COMMON_OBJS)

.PHONY: all cli gui run test clean

all: cli gui

cli: $(CLI_TARGET)

gui: $(GUI_TARGET)

$(CLI_TARGET): $(CLI_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

$(GUI_TARGET): $(GUI_OBJS)
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -o $@ $^ $(GTK_LIBS)

$(BUILD_DIR)/main_cli.o: $(SRC_DIR)/main.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/main_gui.o: $(SRC_DIR)/main.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -DENABLE_GUI $(GTK_CFLAGS) -c $< -o $@

$(BUILD_DIR)/gui.o: $(SRC_DIR)/gui.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

run: cli
	./$(CLI_TARGET)

test: cli
	./$(CLI_TARGET) --test

clean:
	rm -rf $(BUILD_DIR) $(CLI_TARGET) $(GUI_TARGET) $(CLI_TARGET).exe $(GUI_TARGET).exe
