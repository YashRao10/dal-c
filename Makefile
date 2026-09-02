# dal-c -- safety-critical C component library
#
# Plain C99, no external dependencies.
#   make            build and run the test suite
#   make coverage   rebuild instrumented, run tests, emit statement/branch/MC-DC gcov
#   make clean      remove build and coverage artifacts

CC       ?= gcc
AR       ?= ar
PREFIX   ?= /usr/local

# gcov must match the compiler that wrote the .gcno files: a gcc-14 build
# needs gcov-14 (older gcov rejects --conditions and prints its help text
# instead of coverage data). gcc-N -> gcov-N; plain gcc or anything else
# -> gcov. Override with GCOV=... if your toolchain names it differently.
ifneq (,$(filter gcc-%,$(CC)))
GCOV     ?= $(CC:gcc-%=gcov-%)
else
GCOV     ?= gcov
endif
CSTD     := -std=c99
WARN     := -Wall -Wextra -Wpedantic -Werror -Wconversion -Wshadow \
            -Wcast-qual -Wstrict-prototypes -Wmissing-prototypes -Wundef
CPPFLAGS := -Iinclude -Itests
OPT      ?= -O2 -g
CFLAGS    = $(CSTD) $(WARN) $(OPT)

BUILD    := build
SRC      := $(wildcard src/*.c)
TEST_SRC := $(filter-out tests/fuzz.c,$(wildcard tests/*.c))
HDR      := $(wildcard include/*.h) $(wildcard tests/*.h)
SRC_OBJ  := $(patsubst src/%.c,$(BUILD)/%.o,$(SRC))
TEST_OBJ := $(patsubst tests/%.c,$(BUILD)/t_%.o,$(TEST_SRC))
OBJ      := $(SRC_OBJ) $(TEST_OBJ)
TEST_BIN := $(BUILD)/dal-c-tests
LIB      := $(BUILD)/libdal_c.a

.PHONY: all test coverage report lib example install sanitize fuzz clean

all: test

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/%.o: src/%.c $(HDR) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/t_%.o: tests/%.c $(HDR) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(TEST_BIN): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $@

test: $(TEST_BIN)
	./$(TEST_BIN)

# Static library + umbrella header.
lib: $(LIB)

$(LIB): $(SRC_OBJ)
	$(AR) rcs $@ $^

install: lib
	install -d $(DESTDIR)$(PREFIX)/lib $(DESTDIR)$(PREFIX)/include/dal_c
	install -m 644 $(LIB) $(DESTDIR)$(PREFIX)/lib/
	install -m 644 include/*.h $(DESTDIR)$(PREFIX)/include/dal_c/

# Worked example that composes several components; builds and runs it.
example: $(BUILD)/example
	./$(BUILD)/example

# compiled straight from source (not the shared objects) so it always
# reflects the current CFLAGS and never links against instrumented objects
$(BUILD)/example: examples/control_loop.c $(SRC) $(HDR) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) examples/control_loop.c $(SRC) -o $@

# Structural coverage. MC/DC via GCC condition coverage (GCC >= 14).
coverage: OPT := -O0 -g --coverage -fcondition-coverage
coverage: clean $(TEST_BIN)
	./$(TEST_BIN) --report | tee $(BUILD)/test-output.txt
	@echo
	@echo "=== $(GCOV): statement / branch / condition (MC-DC) ==="
	$(GCOV) --branch-counts --conditions --object-directory $(BUILD) $(SRC) \
		| tee $(BUILD)/coverage-summary.txt
	@mkdir -p $(BUILD)/gcov && mv -f *.gcov $(BUILD)/gcov/ 2>/dev/null || true

# Regenerate the published verification artifacts from a real run.
report: coverage
	python3 tools/gen_rtm.py
	python3 tools/gen_report.py
	python3 tools/gen_coverage.py

# Run the suite, the example and the fuzz harness under UBSan + ASan.
sanitize: OPT := -O1 -g -fsanitize=undefined,address -fno-sanitize-recover=all
sanitize: clean $(TEST_BIN) $(BUILD)/example $(BUILD)/fuzz
	./$(TEST_BIN)
	./$(BUILD)/example >/dev/null
	./$(BUILD)/fuzz

# Randomised property / invariant harness.
fuzz: $(BUILD)/fuzz
	./$(BUILD)/fuzz

$(BUILD)/fuzz: tests/fuzz.c $(SRC) $(HDR) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/fuzz.c $(SRC) -o $@

clean:
	rm -rf $(BUILD) *.gcov *.gcda *.gcno
