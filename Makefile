# Convenience commands from the repository root.
.DEFAULT_GOAL := all
.PHONY: all clean cleanobj qemu_start qemu_debug gdb_start tests run_single_test generate_config tools_tests help
all clean cleanobj qemu_start qemu_debug gdb_start tests run_single_test generate_config tools_tests help:
	$(MAKE) -C Project $@
