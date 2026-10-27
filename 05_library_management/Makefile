CC       ?= gcc
CFLAGS   ?= -Wall -Wextra -Wpedantic -std=c11
SANFLAGS  = -fsanitize=address,undefined

COMMON_DIR = ../common
SRC_DIR    = src
TESTS_DIR  = tests
BUILD_DIR  = build

INCLUDES   = -I$(SRC_DIR) -I$(COMMON_DIR)

# Core sources for library binary
LIB_SRCS   = $(SRC_DIR)/book.c $(SRC_DIR)/member.c $(SRC_DIR)/transaction.c \
             $(SRC_DIR)/library.c $(COMMON_DIR)/input.c $(COMMON_DIR)/utils.c \
             $(COMMON_DIR)/file_ops.c
MAIN_SRC   = $(SRC_DIR)/main.c

TARGET     = $(BUILD_DIR)/library

# Test binaries
TEST_BIN_BOOK   = $(BUILD_DIR)/test_book
TEST_BIN_MEMBER = $(BUILD_DIR)/test_member
TEST_BIN_TXN    = $(BUILD_DIR)/test_transaction
TEST_BIN_INTEG  = $(BUILD_DIR)/test_file_integrity

TEST_BINS       = $(TEST_BIN_BOOK) $(TEST_BIN_MEMBER) $(TEST_BIN_TXN) $(TEST_BIN_INTEG)

.PHONY: all clean test test_unit test_integration

all: $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TARGET): $(MAIN_SRC) $(LIB_SRCS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^ -lm

# Build tests with AddressSanitizer and UndefinedBehaviorSanitizer
$(TEST_BIN_BOOK): $(TESTS_DIR)/test_book.c $(LIB_SRCS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(SANFLAGS) $(INCLUDES) -o $@ $^ -lm

$(TEST_BIN_MEMBER): $(TESTS_DIR)/test_member.c $(LIB_SRCS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(SANFLAGS) $(INCLUDES) -o $@ $^ -lm

$(TEST_BIN_TXN): $(TESTS_DIR)/test_transaction.c $(LIB_SRCS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(SANFLAGS) $(INCLUDES) -o $@ $^ -lm

$(TEST_BIN_INTEG): $(TESTS_DIR)/test_file_integrity.c $(LIB_SRCS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(SANFLAGS) $(INCLUDES) -o $@ $^ -lm

test: test_unit test_integration

test_unit: $(TEST_BINS)
	@echo "--- Running test_book (with ASan+UBSan) ---"
	./$(TEST_BIN_BOOK)
	@echo "--- Running test_member (with ASan+UBSan) ---"
	./$(TEST_BIN_MEMBER)
	@echo "--- Running test_transaction (with ASan+UBSan) ---"
	./$(TEST_BIN_TXN)
	@echo "--- Running test_file_integrity (with ASan+UBSan) ---"
	./$(TEST_BIN_INTEG)

test_integration: $(TARGET)
	@echo "--- Running test_menu.sh integration test ---"
	bash $(TESTS_DIR)/test_menu.sh $(TARGET)

clean:
	rm -rf $(BUILD_DIR) data/
