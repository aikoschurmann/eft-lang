# Simplified Makefile
CC := clang
NAME := compiler
OUT_DIR := out
SRC_DIR := src
OBJ_DIR := obj
TEST_DIR := test


# OS specifics
EXE := $(if $(filter Windows_NT,$(OS)),.exe,)
LDFLAGS := -lm $(if $(filter-out Windows_NT,$(OS)),-rdynamic,)

USE_WRAP := 0
ifeq ($(OS),Windows_NT)
    USE_WRAP := 1
else
    ifneq ($(shell uname -s),Darwin)
        USE_WRAP := 1
    endif
endif

TEST_LDFLAGS := $(LDFLAGS)
ifeq ($(USE_WRAP),1)
    TEST_LDFLAGS += -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc
endif


# LLVM config (auto-detect)
LLVM_CONFIG ?= $(shell command -v llvm-config 2>/dev/null || find /opt/homebrew/opt/llvm/bin -name llvm-config 2>/dev/null | head -n 1)
ifeq ($(OS),Windows_NT)
    LLVM_CONFIG ?= C:/Program Files/LLVM/bin/llvm-config.exe
endif

LLVM_CFLAGS := $(shell "$(LLVM_CONFIG)" --cflags 2>/dev/null)
LLVM_LDFLAGS := $(shell "$(LLVM_CONFIG)" --ldflags --libs core analysis bitwriter target native executionengine mcjit orcjit 2>/dev/null)

ifeq ($(LLVM_CFLAGS),)
    $(warning llvm-config not found. LLVM features may fail to compile.)
endif

# Flags
INC_FLAGS := -Iinclude -Iinclude/cli -Iinclude/core -Iinclude/codegen -Iinclude/datastructures -Iinclude/lexing -Iinclude/parsing -Iinclude/sema -Iinclude/types
CFLAGS := $(INC_FLAGS) $(LLVM_CFLAGS) -MMD -MP -g -Wall -Wextra -Wno-unused-parameter \
          -Wshadow -Wstrict-prototypes -Wmissing-prototypes
LDFLAGS += $(LLVM_LDFLAGS)

# Sources
SRCS := $(shell find $(SRC_DIR) -type f -name "*.c" 2>/dev/null)
TEST_SRCS := $(shell find $(TEST_DIR) -type f -name "*.c" 2>/dev/null)

# Objects
OBJS_DEV := $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/dev/%.o)
OBJS_REL := $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/release/%.o)
OBJS_ASAN := $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/asan/%.o)
TEST_OBJS := $(TEST_SRCS:$(TEST_DIR)/%.c=$(OBJ_DIR)/test/%.o)
TEST_OBJS_ASAN := $(TEST_SRCS:$(TEST_DIR)/%.c=$(OBJ_DIR)/asan/test/%.o)

.PHONY: all dev release asan test test-asan clean run run-dev

all: release dev

release: $(OUT_DIR)/$(NAME)$(EXE)
dev: $(OUT_DIR)/$(NAME)-dev$(EXE)
asan: $(OUT_DIR)/$(NAME)-asan$(EXE)

# --- Build Rules Templates ---
define LINK_RULE
$(1): $(2)
	@mkdir -p $$(dir $$@)
	@echo "  LD      $$@"
	@$$(CC) $$^ -o $$@ $$(LDFLAGS) $(3)
endef

$(eval $(call LINK_RULE,$(OUT_DIR)/$(NAME)$(EXE),$(OBJS_REL),))
$(eval $(call LINK_RULE,$(OUT_DIR)/$(NAME)-dev$(EXE),$(OBJS_DEV),))
$(eval $(call LINK_RULE,$(OUT_DIR)/$(NAME)-asan$(EXE),$(OBJS_ASAN),-fsanitize=address,undefined))

define COMPILE_RULE
$(OBJ_DIR)/$(1)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $$(dir $$@)
	@echo "  CC      $$<"
	@$$(CC) $$(CFLAGS) $(2) -c $$< -o $$@
endef

$(eval $(call COMPILE_RULE,release,-O3))
$(eval $(call COMPILE_RULE,dev,-O0 -DDEV_BUILD))
$(eval $(call COMPILE_RULE,asan,-O1 -fsanitize=address,undefined -fno-omit-frame-pointer -fno-common))

# --- Test Runner ---
$(OBJ_DIR)/test/%.o: $(TEST_DIR)/%.c
	@mkdir -p $(dir $@)
	@echo "  CC      $<"
	@$(CC) $(CFLAGS) -O0 -DDEV_BUILD -I$(TEST_DIR)/harness -I$(TEST_DIR)/helpers -c $< -o $@

$(OBJ_DIR)/asan/test/%.o: $(TEST_DIR)/%.c
	@mkdir -p $(dir $@)
	@echo "  CC      $<"
	@$(CC) $(CFLAGS) -O1 -fsanitize=address,undefined -fno-omit-frame-pointer -I$(TEST_DIR)/harness -I$(TEST_DIR)/helpers -c $< -o $@

$(OUT_DIR)/test_runner$(EXE): $(filter-out $(OBJ_DIR)/dev/main.o,$(OBJS_DEV)) $(TEST_OBJS)
	@mkdir -p $(dir $@)
	@echo "  LD      $@"
	@$(CC) $^ -o $@ $(TEST_LDFLAGS)

$(OUT_DIR)/test_runner-asan$(EXE): $(filter-out $(OBJ_DIR)/asan/main.o,$(OBJS_ASAN)) $(TEST_OBJS_ASAN)
	@mkdir -p $(dir $@)
	@echo "  LD      $@"
	@$(CC) $^ -o $@ $(TEST_LDFLAGS) -fsanitize=address,undefined

test: $(OUT_DIR)/test_runner$(EXE)
	@./$(OUT_DIR)/test_runner$(EXE)

test-asan: $(OUT_DIR)/test_runner-asan$(EXE)
	@./$(OUT_DIR)/test_runner-asan$(EXE)

# --- Utilities ---
FILE ?= test.eft

run: release
	@./$(OUT_DIR)/$(NAME)$(EXE) $(FILE)

run-dev: dev
	@./$(OUT_DIR)/$(NAME)-dev$(EXE) $(FILE)

clean:
	@echo "  CLEAN"
	@rm -rf $(OBJ_DIR) $(OUT_DIR)

-include $(OBJS_DEV:.o=.d) $(OBJS_REL:.o=.d) $(OBJS_ASAN:.o=.d) $(TEST_OBJS:.o=.d) $(TEST_OBJS_ASAN:.o=.d)
