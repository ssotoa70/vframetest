MAJOR=25
MINOR=14
PATCH=0
CFLAGS+=-std=c99 -O2 -Wall -Werror -Wpedantic -pedantic-errors -DMAJOR=$(MAJOR) -DMINOR=$(MINOR) -DPATCH=$(PATCH)
LDFLAGS+=-pthread
SRC_DIR=src
HEADERS := $(wildcard $(SRC_DIR)/*.h)
BUILD_FOLDER=$(PWD)/build

# Core sources (all platforms)
SOURCES_CORE=profile.c frame.c tester.c histogram.c report.c platform.c timing.c

# TUI sources (Unix only - requires termios.h, sys/ioctl.h)
SOURCES_TUI=tui.c tty.c tui_state.c tui_input.c tui_views.c screen.c tui_render.c

# Detect Windows (MinGW/MSYS2)
ifdef MSYSTEM
  SOURCES=$(SOURCES_CORE)
  CFLAGS+=-DNO_TUI
else ifeq ($(OS),Windows_NT)
  SOURCES=$(SOURCES_CORE)
  CFLAGS+=-DNO_TUI
else
  SOURCES=$(SOURCES_CORE) $(SOURCES_TUI)
endif
SRC_FILES=$(addprefix $(SRC_DIR)/,$(SOURCES))
TEST_SOURCES=$(wildcard tests/test_*.c)
OBJECTS=$(addprefix $(BUILD_FOLDER)/,$(SOURCES:.c=.o))
ALL_FILES=$(SRC_DIR)/frametest.c $(SRC_FILES) $(HEADERS) $(TEST_SOURCES)

all: $(BUILD_FOLDER) $(BUILD_FOLDER)/vframetest

release: $(BUILD_FOLDER) $(BUILD_FOLDER)/vframetest
	strip $(BUILD_FOLDER)/vframetest

$(BUILD_FOLDER)/vframetest: $(BUILD_FOLDER)/frametest.o $(BUILD_FOLDER)/libvframetest.a
	$(CC) -o $@ $^ $(LDFLAGS)

$(BUILD_FOLDER)/libvframetest.a: $(OBJECTS)
	$(AR) $(ARFLAGS) $@ $^

$(BUILD_FOLDER)/%.o: $(SRC_DIR)/%.c $(HEADERS)
	$(CC) -c $(CFLAGS) -o $@ $<

$(BUILD_FOLDER):
	install -d $(BUILD_FOLDER)

test:
	make -C tests

test-integration: $(BUILD_FOLDER)/vframetest
	@echo "Running integration tests..."
	VFRAMETEST=$(BUILD_FOLDER)/vframetest tests/integration_test.sh

test-all: test test-integration

dist:
	git archive --prefix="vframetest-$(MAJOR).$(MINOR).$(PATCH)/" HEAD | gzip -9 > "vframetest-$(MAJOR).$(MINOR).$(PATCH).tar.gz"

win:
	BUILD_FOLDER="$(BUILD_FOLDER)/win" ./scripts/build/build_win.sh "$(MAJOR).$(MINOR).$(PATCH)"

win64:
	BUILD_FOLDER="$(BUILD_FOLDER)/win64" CROSS=x86_64-w64-mingw32- ./scripts/build/build_win.sh "$(MAJOR).$(MINOR).$(PATCH)"

coverage:
	CFLAGS="-O0 -fprofile-arcs -ftest-coverage" LDFLAGS="-lgcov" make -C tests test BUILD_FOLDER="$(BUILD_FOLDER)/tests-coverage"
	lcov --capture --directory "$(BUILD_FOLDER)/tests-coverage" --output-file "$(BUILD_FOLDER)/test-coverage.info" -exclude "$(shell realpath "$(PWD)/tests")/*" --exclude "/usr/*"
	genhtml "$(BUILD_FOLDER)/test-coverage.info" --output-directory "$(BUILD_FOLDER)/coverage-report"

format:
	clang-format -i $(ALL_FILES)

clean:
	make -C tests clean
	rm -rf $(BUILD_FOLDER)
	rm -f *.o vframetest vframetest.exe libvframetest.a *.gcno *.gcda *.gcov

.PHONY: all clean release test test-integration test-all dist win win64 coverage format
