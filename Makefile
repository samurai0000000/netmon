##
## Makefile
##
## Copyright (C) 2026, Charles Chiou
##

SHELL := /bin/bash
BUILD_DIR := build
NUM_PROCS := $(shell nproc 2>/dev/null || echo 4)

.PHONY: all test clean distclean submodules setcap

all: submodules
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && (test -f Makefile || cmake .. -DCMAKE_BUILD_TYPE=Release)
	@$(MAKE) -C $(BUILD_DIR) -j$(NUM_PROCS)
	@-sudo -n setcap cap_net_raw=eip $(BUILD_DIR)/netmon 2>/dev/null || true

test: all
	@cmake --build $(BUILD_DIR) --target test

submodules:
	@if [ -f .gitmodules ] && ([ ! -f third_party/json/include/nlohmann/json.hpp ] || [ ! -f third_party/cpp-httplib/httplib.h ]); then \
		git submodule update --init --recursive; \
	fi

setcap:
	@if [ ! -f $(BUILD_DIR)/netmon ]; then \
		echo "Error: $(BUILD_DIR)/netmon not found. Run 'make' first."; \
		exit 1; \
	fi
	sudo setcap cap_net_raw=eip $(BUILD_DIR)/netmon
	@echo "Capabilities successfully verified:"
	@getcap $(BUILD_DIR)/netmon


clean:
	@if [ -d $(BUILD_DIR) ] && [ -f $(BUILD_DIR)/Makefile ]; then \
		$(MAKE) -C $(BUILD_DIR) clean; \
	fi

distclean:
	rm -rf $(BUILD_DIR)
