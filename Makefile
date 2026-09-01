# dal-c -- safety-critical C component library
#
# Plain C99, no external dependencies.
#   make            build and run the test suite
#   make coverage   rebuild instrumented, run tests, emit statement/branch/MC-DC gcov
#   make clean      remove build and coverage artifacts

CC       ?= gcc
CSTD     := -std=c99
WARN     := -Wall -Wextra -Wpedantic -Werror -Wconversion -Wshadow \
            -Wcast-qual -Wstrict-prototypes -Wmissing-prototypes -Wundef
CPPFLAGS := -Iinclude -Itests
OPT      ?= -O2 -g
CFLAGS    = $(CSTD) $(WARN) $(OPT)

BUILD    := build
SRC      := $(wildcard src/*.c)
TEST_SRC := $(wildcard tests/*.c)
HDR      := $(wildcard include/*.h) $(wildcard tests/*.h)
SRC_OBJ  := $(patsubst src/%.c,$(BUILD)/%.o,$(SRC))
TEST_OBJ := $(patsubst tests/%.c,$(BUILD)/t_%.o,$(TEST_SRC))
OBJ      := $(SRC_OBJ) $(TEST_OBJ)
TEST_BIN := $(BUILD)/dal-c-tests

.PHONY: all test coverage clean

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

# Structural coverage. MC/DC via GCC condition coverage (GCC >= 14).
coverage: OPT := -O0 -g --coverage -fcondition-coverage
coverage: clean $(TEST_BIN)
	./$(TEST_BIN)
	@echo
	@echo "=== gcov: statement / branch / condition (MC-DC) ==="
	gcov --branch-counts --conditions --object-directory $(BUILD) $(SRC)
	@mkdir -p $(BUILD)/gcov && mv -f *.gcov $(BUILD)/gcov/ 2>/dev/null || true

clean:
	rm -rf $(BUILD) *.gcov *.gcda *.gcno
