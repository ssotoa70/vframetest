CFLAGS+=-std=c99 -O0 -g -Wall -Werror -Wpedantic -pedantic-errors -I. -I.. -I../src
TESTS=frame histogram profile tester
BUILD_FOLDER:=$(PWD)/build/tests
TEST_BINS=$(addprefix $(BUILD_FOLDER)/test_,$(TESTS))
OBJECTS=$(addsuffix .o,$(TEST_BINS))
DATE=$(shell date +%Y%m%d-%H%M%S)

# Platform-specific linker flags (--wrap is GNU ld, not available on macOS/clang)
UNAME_S=$(shell uname -s)
ifeq ($(UNAME_S),Linux)
	LDFLAGS+=-Wl,--wrap=printf -Wl,--wrap=puts -Wl,--wrap=putchar
endif
ifeq ($(UNAME_S),Darwin)
	# macOS: use alias instead of wrap (not supported, skip for now)
	# Tests will still work without symbol wrapping on macOS
endif

all: test

test: build_tests run_tests

build_tests: $(BUILD_FOLDER) $(BUILD_FOLDER)/test_platform.o $(OBJECTS)

$(BUILD_FOLDER):
	install -d $(BUILD_FOLDER)

$(BUILD_FOLDER)/test_tester: $(BUILD_FOLDER)/test_platform.o $(BUILD_FOLDER)/test_tester.o $(BUILD_FOLDER)/platform.o
	$(CC) -o $@ $^ $(LDFLAGS)

$(BUILD_FOLDER)/test_%: $(BUILD_FOLDER)/test_platform.o $(BUILD_FOLDER)/test_%.o
	$(CC) -o $@ $^ $(LDFLAGS)

$(BUILD_FOLDER)/platform.o: ../src/platform.c ../src/platform.h
	$(CC) -c $(CFLAGS) -o $@ $<

$(BUILD_FOLDER)/test_%.o: test_%.c ../src/%.c ../src/%.h test_platform.c
	$(CC) -c $(CFLAGS) -o $@ $<

run_tests: $(TEST_BINS)
	@for tst in $(TESTS); do \
		"$(BUILD_FOLDER)/test_$${tst}"; \
	done

clean:
	rm -f *.o *.gcno *.gcda *.gcov $(TEST_BINS)

.PHONY: all test build_tests run_tests
