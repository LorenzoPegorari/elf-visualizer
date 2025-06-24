# Makefile created by Lorenzo Pegorari


# -------------------- VARIABLES --------------------
# User-defined variables
SRC_DIR := src
INC_DIR := include
BUILD_DIR := build
OBJS_DIR := objs
DEPS_DIR := deps

BIN := elf-visualizer

SRCS := $(shell find $(SRC_DIR) -name '*.c')
OBJS := $(addprefix $(BUILD_DIR)/, $(subst $(SRC_DIR), $(OBJS_DIR), $(SRCS:.c=.o)))
DEPS := $(addprefix $(BUILD_DIR)/, $(subst $(SRC_DIR), $(DEPS_DIR), $(SRCS:.c=.deps)))

INC_FLAGS := -I$(INC_DIR)

# Standard variables
CC := gcc
CFLAGS := -Wall -Wextra -pedantic -std=c89 -no-pie $(INC_FLAGS)
LDFLAGS := -lc


# -------------------- GOALS --------------------
.PHONY: release clean

# RELEASE
release: $(BUILD_DIR)/$(BIN)

# Linking
$(BUILD_DIR)/$(BIN): $(OBJS) $(DEPS)
	mkdir -p $(dir $@)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

# Compiling
$(BUILD_DIR)/$(OBJS_DIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(dir $@)
	$(CC) $< -o $@ -c $(CFLAGS)


# DEPENDENCIES
$(BUILD_DIR)/$(DEPS_DIR)/%.deps: $(SRC_DIR)/%.c
	mkdir -p $(dir $@)
	$(CC) -M $(CFLAGS) $< > $@


# PHONIES
clean:
	rm -r $(BUILD_DIR)


# -------------------- INCLUDE --------------------
# Include dependencies
ifneq ($(MAKECMDGOALS), clean)
-include $(DEPS)
endif
