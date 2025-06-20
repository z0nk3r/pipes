# Makefile for executable
.PHONY: all debug clean check valgrind helgrind tidy format sanitize_a sanitize_t
# *****************************************************
# Parameters to control Makefile operation
BIN := $(shell grep "main (.*)" src/*.c -l | cut -f2 -d/ | cut -f1 -d.)

BIN_DIR := bin
SRC_DIR := src
OBJ_DIR := obj
INC_DIR := include
TST_DIR := test

SRCS := $(wildcard $(SRC_DIR)/*.c)
# SRCS := $(wildcard *.c)
OBJS := $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

# CC := gcc-9
# STANDARD := -std=c18
STANDARD := -std=c99
FEATURES := -D_DEFAULT_SOURCE
# FEATURES := -D_POSIX_C_SOURCE=200809L
# CFLAGS := -Wall -Wextra -Wpedantic -Wno-scalar-storage-order
# CFLAGS += -Wwrite-strings -Wvla -Wfloat-equal -Waggregate-return
CFLAGS += $(STANDARD) $(FEATURES)

VFLAGS := -s --track-origins=yes --leak-check=full --show-leak-kinds=all

HFLAGS := --tool=helgrind --ignore-thread-creation=yes --history-level=none

BIN_ARGS := -c

# ****************************************************
# Entries to bring the executable up to date

all: $(BIN)

test: $(BIN)
	@./$(BIN_DIR)/$(BIN) $(BIN_ARGS)

backtrace: CFLAGS += -g
backtrace: clean $(BIN)
	gdb -ex="set confirm off" -ex r -ex bt -q --args ./$(BIN_DIR)/$(BIN) $(BIN_ARGS)

clean:
	@rm -rf $(OBJ_DIR) $(BIN_DIR)

debug: CFLAGS += -g3 -DDEBUG
debug: $(BIN)

profile: CFLAGS += -g3 -pg
profile: $(BIN)

valgrind: CFLAGS += -g3
valgrind: clean $(BIN)
valgrind:
	@valgrind $(VFLAGS) ./$(BIN_DIR)/$(BIN) $(BIN_ARGS)

helgrind: CFLAGS += -g3
helgrind: clean $(BIN)
helgrind:
	@valgrind $(HFLAGS) ./$(BIN_DIR)/$(BIN) $(BIN_ARGS)

format:
	@clang-format -i $(SRCS)

tidy:
	@clang-tidy -checks="cert*, bugprone*, misc*, readability*" --warnings-as-errors=* src/*.c -- $(STANDARD) -I$(INC_DIR)

complexity:
	@complexity --score --threshold=0 $(SRCS)

cppcheck:
	@cppcheck --enable=all $(SRCS)

sanitize_a: CFLAGS += -fsanitize=address -static-libasan -g
sanitize_a: $(BIN)

sanitize_t: CFLAGS += -fsanitize=thread -g
sanitize_t: $(BIN)

$(OBJ_DIR):
	@mkdir -p $@

$(BIN_DIR):
	@mkdir -p $@

$(OBJS): | $(OBJ_DIR)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@$(CC) $(CFLAGS) -I$(INC_DIR) -c $< -o $@

$(OBJ_DIR)/%.o: $(TST_DIR)/%.c
	@$(CC) $(CFLAGS) -I$(INC_DIR) -c $< -o $@

$(BIN): $(OBJS) | $(BIN_DIR)
	@$(CC) $(CFLAGS) $^ -o $(BIN_DIR)/$@ -lm
