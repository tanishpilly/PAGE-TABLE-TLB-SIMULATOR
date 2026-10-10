# Page Table & TLB Simulator

**B.Tech Semester 4 Operating Systems & System Programming Problem-Based Learning (PBL) Project**

---

## Team Members

* **2510030055** — Tanish Pilly
* **2510030265** — Eshwar Sidhartha
* **2510030199** — G Manikanta

---

## 1. Project Overview

The **Page Table & TLB Simulator** is an operating systems memory management simulator written in C. It models the Memory Management Unit (MMU) address translation process and Translation Lookaside Buffer (TLB) caching mechanisms. It features both a full-featured **Interactive Terminal CLI** and a modern **GTK Desktop GUI Dashboard**.

---

## 2. Key Features

* **Bitwise Address Decomposition**: Dynamically extracts page number and page offset using bitwise shift (`>>`) and mask (`&`) operations.
* **TLB Cache Simulation**: Implements a high-speed TLB cache with Least Recently Used (LRU) eviction policy.
* **Page Table Simulation**: Supports virtual-to-physical frame mappings, valid/invalid bit tracking, and unmapped page fault detection.
* **Modern GTK 3 Desktop GUI**: Dark navy dashboard featuring interactive address translation, step-by-step visual translation flow, live statistics, page table and TLB inspection tables.
* **Interactive Terminal CLI**: Classic menu system for headless environments or terminal demonstrations.
* **Automated Test Suite**: Built-in test harness executing official validation test cases.
* **Faculty Demonstration Mode**: Guided step-by-step walkthrough showing TLB MISS, TLB HIT, and Page Fault sequences.

---

## 3. Technologies & Dependencies

* **Language**: C (C99 Standard)
* **GUI Toolkit**: GTK 3 (`libgtk-3-dev`, `pkg-config`)
* **Compiler**: GCC (`-Wall -Wextra -std=c99 -pedantic`)
* **Build System**: GNU Make

### Ubuntu / Debian Package Installation:
```bash
sudo apt update
sudo apt install build-essential pkg-config libgtk-3-dev
```

---

## 4. Project Architecture & Structure

```
PageTableTLBSimulator/
│
├── src/
│   ├── main.c          # Shared entry point for CLI and GUI modes
│   ├── gui.h           # GTK Dashboard header
│   ├── gui.c           # GTK 3 Graphical User Interface implementation
│   ├── page_table.h    # Page table header
│   ├── page_table.c    # Page table implementation
│   ├── tlb.h           # TLB cache header
│   ├── tlb.c           # TLB implementation with LRU replacement
│   ├── simulator.h     # Core simulator header & config structures
│   └── simulator.c     # Address translation engine, tests, and demo mode
│
├── tests/
│   └── test_cases.txt  # Documented test case specifications
│
├── docs/
│   └── README.md       # Project documentation & user guide
│
├── Makefile            # Build, test, run, and clean tasks
└── .gitignore          # Git ignore rules
```

---

## 5. Address Translation Algorithm & Formulas

Given a `virtual_address` and a `page_size` of 4096 bytes:

1. **Calculate Offset Bits**:  
   $$\text{offset\_bits} = \log_2(\text{page\_size}) = 12$$

2. **Extract Page Number**:  
   $$\text{page\_number} = \text{virtual\_address} \gg 12$$

3. **Extract Offset**:  
   $$\text{offset} = \text{virtual\_address} \ \& \ (\text{page\_size} - 1) = \text{virtual\_address} \ \& \ 4095$$

4. **Lookup Frame**:  
   - First check TLB cache.
   - If TLB Miss, check Page Table.

5. **Compute Physical Address**:  
   $$\text{physical\_address} = (\text{frame} \ll 12) \mid \text{offset}$$

---

## 6. TLB Behavior & Page Fault Handling

* **TLB MISS**: On initial virtual page access, the entry is absent from the TLB. The MMU queries the Page Table. If mapped, the frame number is retrieved and inserted into the TLB using LRU replacement.
* **TLB HIT**: On subsequent accesses to the same virtual page, the frame is retrieved directly from the high-speed TLB cache, bypassing the main Page Table.
* **PAGE FAULT**: Triggered when a requested page is outside the virtual page bounds or marked invalid (`valid = false`). Displays `PAGE FAULT: Unmapped Address` without crashing.

---

## 7. How to Build & Run

### Build All Executables (`pagetable_tlb_sim` & `pagetable_tlb_gui`):
```bash
make
```

### Build Only CLI Executable:
```bash
make cli
```

### Build Only GUI Executable:
```bash
make gui
```

### Launch GUI Dashboard:
```bash
./pagetable_tlb_gui
```
or
```bash
make gui && ./pagetable_tlb_gui
```

### Launch CLI Interactive Menu:
```bash
./pagetable_tlb_sim
```
or
```bash
make run
```

### Run Automated Test Suite:
```bash
make test
```
or
```bash
./pagetable_tlb_sim --test
```

### Clean Build Output:
```bash
make clean
```

---

## 8. Sample Translation Example

### Address `4098`:
- Virtual Address: `4098`
- Page Number: `4098 >> 12 = 1`
- Offset: `4098 & 4095 = 2`
- Page Table Mapping: Page `1` $\rightarrow$ Frame `5`
- Physical Address: `(5 << 12) | 2 = 20480 + 2 = 20482 (0x5002)`

1. **First Access (4098)**: TLB Status = `MISS`, Page Table = `HIT`, Frame = `5`, Physical Address = `20482`
2. **Second Access (4098)**: TLB Status = `HIT`, Page Table = `BYPASSED`, Frame = `5`, Physical Address = `20482`
3. **Unmapped Access (8192)**: TLB Status = `MISS`, Page Table = `FAULT`, Status = `PAGE FAULT: Unmapped Address`

---

## 9. Faculty Verification Checklist

- [x] Backend C simulator preserved without modifying translation math
- [x] GTK 3 graphical user interface connected to backend C API
- [x] Modern dark navy OS memory management dashboard design
- [x] Dynamic visual translation flow diagram
- [x] Live statistics, Page Table grid, and TLB grid
- [x] Automated test runner dialog and CLI test runner
- [x] Faculty demonstration mode (CLI `--demo` and GUI `DEMO MODE` button)
- [x] Compiles cleanly with `-Wall -Wextra -std=c99 -pedantic`
