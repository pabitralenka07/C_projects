CC       ?= gcc
CFLAGS   ?= -Wall -Wextra -Wpedantic -std=c11
SANFLAGS  = -fsanitize=address,undefined

COMMON_DIR = ../common
SRC_DIR    = src
TESTS_DIR  = tests
BUILD_DIR  = build

INCLUDES   = -I$(SRC_DIR) -I$(COMMON_DIR)

TODO_SRCS  = $(SRC_DIR)/todo.c $(COMMON_DIR)/input.c $(COMMON_DIR)/utils.c $(COMMON_DIR)/file_ops.c
MAIN_SRC   = $(SRC_DIR)/main.c

TARGET     = $(BUILD_DIR)/todo
TEST_BIN   = $(BUILD_DIR)/test_todo

.PHONY: all clean test test_unit test_integration

all: $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TARGET): $(MAIN_SRC) $(TODO_SRCS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^

$(TEST_BIN): $(TESTS_DIR)/test_todo.c $(TODO_SRCS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(SANFLAGS) $(INCLUDES) -o $@ $^

test: test_unit test_integration

test_unit: $(TEST_BIN)
	@echo "--- Running test_todo (with ASan+UBSan) ---"
	./$(TEST_BIN)

test_integration: $(TARGET)
	@echo "--- Running test_menu.sh integration test ---"
	bash $(TESTS_DIR)/test_menu.sh $(TARGET)

clean:
	rm -rf $(BUILD_DIR) data/
