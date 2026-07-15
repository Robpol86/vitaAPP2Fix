.DEFAULT_GOAL := help
PROJECT_NAME := vitaQmBluetooth

## Build

build-%/compile_commands.json: FIRST_DIR = $(firstword $(subst /, ,$@))
build-%/compile_commands.json: CMAKE_BUILD_TYPE = $(if $(filter build-release,$(FIRST_DIR)),Release,Debug)
build-%/compile_commands.json: EXTRA_CMAKE_ARGS ?=
build-%/compile_commands.json: CMakeLists.txt $(wildcard */CMakeLists.txt cmake/*.cmake)
	cmake -B $(FIRST_DIR) -DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE) $(EXTRA_CMAKE_ARGS)

DEBUG_TARGETS := build-debug/module_user/$(PROJECT_NAME).suprx build-debug/module_kernel/$(PROJECT_NAME).skprx
RELEASE_TARGETS := build-release/module_user/$(PROJECT_NAME).suprx build-release/module_kernel/$(PROJECT_NAME).skprx
BUILD_SRC_FILES = $(wildcard include/* module_*/* module_*/*/* module_*/*/*/* module_*/*/*/*/*)

$(DEBUG_TARGETS): build-debug/compile_commands.json
$(RELEASE_TARGETS): build-release/compile_commands.json
$(DEBUG_TARGETS) $(RELEASE_TARGETS): $(BUILD_SRC_FILES)
	cmake --build $(firstword $(subst /, ,$@))

.PHONY: build
build: _HELP = Build debug and release plugins
build: $(DEBUG_TARGETS) $(RELEASE_TARGETS)

## PS Vita

.PHONY: deploy-init
deploy-init: _HELP = Deploy the plugin's dependencies to the PS Vita (requires vitacompanion)
deploy-init:
ifndef PSVITA_IP
	$(error PSVITA_IP is not set. Install https://github.com/devnoname120/vitacompanion on the Vita and set PSVITA_IP.")
endif
	curl -T redist/quickmenureborn/QuickMenuReborn.suprx "ftp://$(PSVITA_IP):1337/ur0:/tai/"
	lftp -p 1337 "$(PSVITA_IP)" -e "mkdir -f /ur0:/QuickMenuReborn; bye"
	curl -T redist/quickmenureborn/qmr_plugin.rco "ftp://$(PSVITA_IP):1337/ur0:/QuickMenuReborn/"

.PHONY: deploy
deploy: _HELP = Deploy plugin to the PS Vita (requires vitacompanion)
deploy: $(DEBUG_TARGETS)
ifndef PSVITA_IP
	$(error PSVITA_IP is not set. Install https://github.com/devnoname120/vitacompanion on the Vita and set PSVITA_IP.")
endif
	curl -T $(<) "ftp://$(PSVITA_IP):1337/ur0:/QuickMenuReborn/"
	curl -T $(word 2,$^) "ftp://$(PSVITA_IP):1337/ur0:/tai/"
	echo screen on |nc -v "$(PSVITA_IP)" 1338

.PHONY: reboot
reboot: _HELP = Reboot the PS Vita (requires vitacompanion)
reboot:
ifndef PSVITA_IP
	$(error PSVITA_IP is not set. Install https://github.com/devnoname120/vitacompanion on the Vita and set PSVITA_IP.")
endif
	@echo "Rebooting in 3 seconds..."
	@sleep 3
	echo reboot |nc -v "$(PSVITA_IP)" 1338

.PHONY: fetch-screenshots
fetch-screenshots: _HELP = Remotely flatten screenshots dir then download (requires vitacompanion)
fetch-screenshots:
ifndef PSVITA_IP
	$(error PSVITA_IP is not set. Install https://github.com/devnoname120/vitacompanion on the Vita and set PSVITA_IP.")
endif
	lftp -p 1337 "$(PSVITA_IP)" -e "set ftp:list-empty-ok yes; cd /ux0:/picture/SCREENSHOT/ && mmv */*.png .; bye"
	wget --quiet -P $(@) -nH --cut-dirs=3 --mirror "ftp://$(PSVITA_IP):1337/ux0:/picture/SCREENSHOT/*.png"

.PHONY: fetch-logs
fetch-logs: _HELP = Download project log files (requires vitacompanion)
fetch-logs:
ifndef PSVITA_IP
	$(error PSVITA_IP is not set. Install https://github.com/devnoname120/vitacompanion on the Vita and set PSVITA_IP.")
endif
	wget --quiet -P $(@) -nH --cut-dirs=3 --mirror "ftp://$(PSVITA_IP):1337/ux0:/$(PROJECT_NAME)/logs/*.log"

.PHONY: tail-todays-log
tail-todays-log: _HELP = Print the last $NUMLINES in today's log file (calls fetch-logs)
tail-todays-log: NUMLINES = 50
tail-todays-log: DATE = $(shell date +%Y%m%d)
tail-todays-log: fetch-logs
	tail -n$(NUMLINES) $(<)/vitaQmBluetooth-$(DATE).log

.PHONY: recv-logs
recv-logs: _HELP = Listen for logs sent from the PS Vita, print to stdout (use with Cat-A-Log)
recv-logs:
	nc -kl 10224

## Testing

build-test/compile_commands.json: EXTRA_CMAKE_ARGS = -DCMAKE_C_COMPILER=clang -DUNIT_TESTING=ON

BUILD_TEST_FILES = $(wildcard tests/* tests/*/* tests/*/*/* tests/*/*/*/*)
LINT_FORMAT_EXTS := %.c %.cpp %.h %.h.in
LINT_FORMAT_FILES_SRC = $(filter $(LINT_FORMAT_EXTS),$(BUILD_SRC_FILES))
LINT_FORMAT_FILES_TESTS = $(filter $(LINT_FORMAT_EXTS),$(BUILD_TEST_FILES))

.PHONY: lint
lint: _HELP = Run linters
lint: build-debug/compile_commands.json build-test/compile_commands.json
	clang-tidy --quiet -p build-debug $(LINT_FORMAT_FILES_SRC)
	clang-tidy --quiet -p build-test $(LINT_FORMAT_FILES_TESTS)
	clang-format --dry-run --Werror $(LINT_FORMAT_FILES_SRC) $(LINT_FORMAT_FILES_TESTS)

.PHONY: format
format: _HELP = Apply format/lint fixes
format:
	clang-format -i $(LINT_FORMAT_FILES_SRC) $(LINT_FORMAT_FILES_TESTS)

.PHONY: test
test: _HELP = Run unit tests
test: build-test/compile_commands.json
	cmake --build build-test --target coverage

## Misc

.PHONY: all
all: _HELP = Run linters and unit tests and then build
all: $(DEBUG_TARGETS) $(RELEASE_TARGETS) test lint

.PHONY: clean
clean: _HELP = Remove build files
clean:
	rm -rfv CMakeCache.txt CMakeFiles/
ifneq ($(wildcard build-*),)
	find build-*/ -mindepth 1 -maxdepth 1 ! -iname _deps -exec rm -rfv {} \;
endif

.PHONY: distclean
distclean: _HELP = Remove build, temporary, and cached files
distclean: clean
	rm -rfv build-*/ fetch-*/

define MAKEFILE_HELP_AWK
BEGIN {
	while (getline < "/dev/stdin") if ($$0 == "# Files") break  # Skip lines until targets start being listed.
	while (getline < "/dev/stdin") {
		if ($$0 ~ /^# makefile \(from [`']/ && match($$0, /', line [0-9]+\)$$/)) {
			file_name = substr($$0, 19, RSTART - 19)
			line_no = substr($$0, RSTART + 8, RLENGTH - 8 - 1)
		} else if (match($$0, /^# _HELP = /) || match($$0, /^[^ \t#:]+: _HELP = /)) {
			help = substr($$0, RLENGTH + 1)
			if (RLENGTH > 10) target_name = $$1  # make 4.x
			if (file_name && line_no && target_name && help) {  # Commit to data array if all states are set.
				for (i = 0; file_name SUBSEP line_no SUBSEP i SUBSEP "target_name" in data; i++) {}  # Just increment i.
				data[file_name,line_no,i,"target_name"] = target_name
				data[file_name,line_no,i,"help"] = help
				if (length(target_name) > data["width"]) data["width"] = length(target_name)
				$$0 = ""  # Signal to clear state.
			}
		} else if (match($$0, /^[^ \t#:]+:/)) target_name = $$1  # make 3.
		if (!$$0) file_name = line_no = target_name = help = ""  # Reset state on blank line.
	}

	data["width"] += col1pad + 0  # Adding 0 initializes to int.
	if (col1minwidth + 0 > data["width"]) data["width"] = col1minwidth
	if (col1tab + 0 && (remainder = data["width"] % col1tab)) data["width"] += (col1tab - remainder)

	cyan    = (colors == "true") ? "\033[36m" : ""
	magenta = (colors == "true") ? "\033[35m" : ""
	reset   = (colors == "true") ? "\033[00m" : ""
}

match($$0, /^[ \t]*##[ \t]*[^ \t#]/) {
	gsub(/[ \t]+$$/, "")  # Strip trailing whitespace.
	header = substr($$0, RLENGTH)
}

FILENAME SUBSEP FNR SUBSEP 0 SUBSEP "target_name" in data {
	if (header) { print magenta header ":" reset; header = "" }
	target_fmt = "%-" data["width"] "s"
	for (i = 0; FILENAME SUBSEP FNR SUBSEP i SUBSEP "target_name" in data; i++) {
		printf(cyan target_fmt reset "%s\n", data[FILENAME,FNR,i,"target_name"], data[FILENAME,FNR,i,"help"])
	}
}
endef

.PHONY: help
help: make_workaround = $(MAKE)
help: export program = $(MAKEFILE_HELP_AWK)
help: _HELP = Print help menu
help:
	@($(make_workaround) -qprR $(foreach f,$(MAKEFILE_LIST),-f $(f)) 2>/dev/null || true) |awk -F : \
		-v "col1minwidth=$(or $(HELP_WIDTH),16)" \
		-v "col1tab=$(or $(HELP_TAB),4)" \
		-v "col1pad=$(or $(HELP_PAD),2)" \
		-v "colors=$(or $(HELP_COLORS),true)" \
		"$$program" $(MAKEFILE_LIST)
