# PTL for the QEMU MPS2-AN385 / Cortex-M3.
# Requires the complete Arm GNU Toolchain with Newlib.
.DEFAULT_GOAL := all

TOOLCHAIN_PREFIX ?= arm-none-eabi-
CC := $(TOOLCHAIN_PREFIX)gcc
SIZE := $(TOOLCHAIN_PREFIX)size
GDB ?= $(TOOLCHAIN_PREFIX)gdb
QEMU ?= qemu-system-arm
PYTHON ?= python3
FREERTOS_ROOT := ../FreeRTOS
PORT_DIR := $(FREERTOS_ROOT)/portable/GCC/ARM_CM3
ENTRY_POINT ?= main.c
# Each entry point has its own artifacts, preventing stale demo/test binaries.
OUTPUT_DIR := Output/$(basename $(notdir $(ENTRY_POINT)))
ELF := $(OUTPUT_DIR)/Project.elf
MAP := $(OUTPUT_DIR)/Project.map
MACHINE := mps2-an385
CPU := cortex-m3
QEMU_FLAGS := -machine $(MACHINE) -cpu $(CPU) -monitor none -nographic -serial stdio -icount 1

SOURCES := $(FREERTOS_ROOT)/list.c $(FREERTOS_ROOT)/tasks.c $(FREERTOS_ROOT)/queue.c
SOURCES += $(FREERTOS_ROOT)/portable/MemMang/heap_4.c $(PORT_DIR)/port.c
SOURCES += Utils/uart.c Utils/burner.c startup.c
SOURCES += PTL/source/ptl_trace.c PTL/source/ptl_wrapper.c PTL/source/ptl_scheduler.c
SOURCES += $(ENTRY_POINT)
VPATH := $(sort $(dir $(SOURCES)))
OBJECTS := $(addprefix $(OUTPUT_DIR)/,$(notdir $(SOURCES:.c=.o)))
CPPFLAGS += -I$(FREERTOS_ROOT)/include -I$(PORT_DIR) -I. -IUtils -IPTL/include
CFLAGS += -ffreestanding -mcpu=$(CPU) -mthumb -Wall -Wextra -Wshadow
CFLAGS += -g3 -Os -ffunction-sections -fdata-sections -MMD -MP
LDFLAGS += -T mps2_m3.ld -nostartfiles -specs=nano.specs -specs=nosys.specs
LDFLAGS += -Wl,-Map=$(MAP),--gc-sections -mcpu=$(CPU) -mthumb

TEST_ORDER := Tests/test_init_safety.c Tests/test_safety_limits.c
TEST_ORDER += Tests/test_policy_kill_trace.c Tests/test_policy_skip_trace.c
TEST_ORDER += Tests/test_policy_catchup_trace.c Tests/test_preemption.c
TEST_ORDER += Tests/test_edge_minimal_gap.c Tests/test_mixed_trace.c
TEST_ORDER += Tests/test_production_check.c Tests/test_load_on_trace.c
TEST_DURATION ?= 5
SHOW ?= 0
SHOW_ON_SCREEN ?= 0

.PHONY: all clean cleanobj qemu_start qemu_debug gdb_start tests run_single_test generate_config tools_tests help
.NOTPARALLEL: tests run_single_test
all: $(ELF)

$(ELF): $(OBJECTS) mps2_m3.ld Makefile
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@
	$(SIZE) $@

$(OUTPUT_DIR)/%.o: %.c Makefile | $(OUTPUT_DIR)
	@echo " [CC] $<"
	@$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(OUTPUT_DIR):
	mkdir -p $@

-include $(OBJECTS:.o=.d)

cleanobj:
	$(RM) $(OBJECTS) $(OBJECTS:.o=.d)

clean:
	$(RM) -r Output

qemu_start: all
	$(QEMU) $(QEMU_FLAGS) -kernel $(ELF)

qemu_debug: all
	$(QEMU) $(QEMU_FLAGS) -kernel $(ELF) -s -S

gdb_start: all
	$(GDB) -ex "target remote localhost:1234" $(ELF)

run_single_test: all
	$(PYTHON) Tools/run_test.py --qemu "$(QEMU)" --elf "$(ELF)" \
		--name "$(notdir $(ENTRY_POINT))" --duration "$(TEST_DURATION)" \
		--show "$(SHOW)" --show-on-screen "$(SHOW_ON_SCREEN)"

tests:
	@mkdir -p logs
	@: > qemu_test.log
	@for test in $(TEST_ORDER); do \
		echo "\n[TESTING] $$test"; \
		$(MAKE) --no-print-directory run_single_test ENTRY_POINT=$$test || exit 1; \
	done
	@echo "\nAll 10 regression tests passed. Logs: logs/ and qemu_test.log"

tools_tests:
	$(PYTHON) -m unittest discover -s Tools -p 'test_*.py' -v

generate_config:
	$(PYTHON) Tools/framework.py

help:
	@echo "make all                 Build the selected entry point (default: main.c)"
	@echo "make qemu_start          Build and run; exit QEMU with Ctrl+A, then X"
	@echo "make tests               Run all regression tests"
	@echo "make run_single_test ENTRY_POINT=Tests/test_policy_kill_trace.c"
	@echo "make generate_config     Open the interactive configuration tool"
	@echo "make qemu_debug          Start QEMU paused, GDB on localhost:1234"
	@echo "make gdb_start           Connect GDB from a second terminal"
	@echo "make clean               Remove build artifacts (keep logs and generated C)"
