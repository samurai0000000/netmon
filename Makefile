##
## Makefile
##
## Copyright (C) 2026, Charles Chiou
##

SHELL := /bin/bash
BUILD_DIR := build
NUM_PROCS := $(shell nproc 2>/dev/null || echo 4)

.PHONY: all test clean distclean submodules setcap setcap_netmon

all: submodules
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && (test -f Makefile || cmake .. -DCMAKE_BUILD_TYPE=Release)
	@$(MAKE) -C $(BUILD_DIR) -j$(NUM_PROCS)
	@if [ -u $(BUILD_DIR)/setcap_netmon ] && [ "$$(stat -c '%u' $(BUILD_DIR)/setcap_netmon 2>/dev/null)" = "0" ]; then \
		$(BUILD_DIR)/setcap_netmon $(BUILD_DIR)/netmon; \
	fi

setcap_netmon: src/setcap_netmon.c
	@mkdir -p $(BUILD_DIR)
	@if [ -u $(BUILD_DIR)/setcap_netmon ] && [ "$$(stat -c '%u' $(BUILD_DIR)/setcap_netmon 2>/dev/null)" = "0" ]; then \
		echo "$(BUILD_DIR)/setcap_netmon is already active (root-owned, setuid). Nothing to compile."; \
	else \
		rm -f $(BUILD_DIR)/setcap_netmon 2>/dev/null || true; \
		$(CC) -O2 src/setcap_netmon.c -o $(BUILD_DIR)/setcap_netmon; \
		echo "Built $(BUILD_DIR)/setcap_netmon. To activate setuid capability, run once:"; \
		echo "  sudo chown root:root $(BUILD_DIR)/setcap_netmon && sudo chmod 4755 $(BUILD_DIR)/setcap_netmon"; \
	fi

test: all
	@cmake --build $(BUILD_DIR) --target test

submodules:
	@if [ -f .gitmodules ] && ([ ! -f third_party/json/include/nlohmann/json.hpp ] || [ ! -f third_party/cpp-httplib/httplib.h ]); then \
		git submodule update --init --recursive; \
	fi

setcap:
	@if [ -u $(BUILD_DIR)/setcap_netmon ] && [ "$$(stat -c '%u' $(BUILD_DIR)/setcap_netmon 2>/dev/null)" = "0" ]; then \
		$(BUILD_DIR)/setcap_netmon $(BUILD_DIR)/netmon; \
	elif [ -f $(BUILD_DIR)/netmon ]; then \
		sudo setcap cap_net_raw=eip $(BUILD_DIR)/netmon; \
		echo "Capabilities successfully verified:"; \
		getcap $(BUILD_DIR)/netmon; \
	else \
		echo "Error: $(BUILD_DIR)/netmon not found. Run 'make' first."; \
		exit 1; \
	fi

clean:
	@if [ -d $(BUILD_DIR) ] && [ -f $(BUILD_DIR)/Makefile ]; then \
		$(MAKE) -C $(BUILD_DIR) clean; \
	fi

distclean:
	@if [ -u $(BUILD_DIR)/setcap_netmon ] && [ "$$(stat -c '%u' $(BUILD_DIR)/setcap_netmon 2>/dev/null)" = "0" ]; then \
		$(BUILD_DIR)/setcap_netmon --clean 2>/dev/null || true; \
	fi
	@-rm -f $(BUILD_DIR)/setcap_netmon 2>/dev/null || true
	rm -rf $(BUILD_DIR)
