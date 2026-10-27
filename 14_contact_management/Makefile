CC       ?= gcc
CFLAGS   ?= -Wall -Wextra -Wpedantic -std=c11 -D_POSIX_C_SOURCE=200809L
SANFLAGS  = -fsanitize=address,undefined

COMMON_DIR = ../common
SRC_DIR    = src
TESTS_DIR  = tests
BUILD_DIR  = build

INCLUDES   = -I$(SRC_DIR) -I$(COMMON_DIR)

COMMON_SRCS = $(COMMON_DIR)/safe_utils.c $(COMMON_DIR)/file_ops.c \
              $(COMMON_DIR)/input.c $(COMMON_DIR)/utils.c $(COMMON_DIR)/hash.c
CORE_SRCS   = $(SRC_DIR)/contact.c $(SRC_DIR)/contact_csv.c $(COMMON_SRCS)
MAIN_SRC    = $(SRC_DIR)/main.c

TARGET      = $(BUILD_DIR)/contact_manager
TEST_CONTACT   = $(BUILD_DIR)/test_contact
TEST_CSV       = $(BUILD_DIR)/test_csv
TEST_INTEGRITY = $(BUILD_DIR)/test_file_integrity

.PHONY: all clean test test_unit test_integration

all: $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TARGET): $(MAIN_SRC) $(CORE_SRCS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^

$(TEST_CONTACT): $(TESTS_DIR)/test_contact.c $(CORE_SRCS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(SANFLAGS) $(INCLUDES) -o $@ $^

$(TEST_CSV): $(TESTS_DIR)/test_csv.c $(CORE_SRCS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(SANFLAGS) $(INCLUDES) -o $@ $^

$(TEST_INTEGRITY): $(TESTS_DIR)/test_file_integrity.c $(CORE_SRCS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(SANFLAGS) $(INCLUDES) -o $@ $^

test: test_unit test_integration

test_unit: $(TEST_CONTACT) $(TEST_CSV) $(TEST_INTEGRITY)
	@echo "--- Running test_contact (with ASan+UBSan) ---"
	./$(TEST_CONTACT)
	@echo "--- Running test_csv (with ASan+UBSan) ---"
	./$(TEST_CSV)
	@echo "--- Running test_file_integrity (with ASan+UBSan) ---"
	./$(TEST_INTEGRITY)

test_integration: $(TARGET)
	@echo "--- Running test_menu.sh integration test ---"
	bash $(TESTS_DIR)/test_menu.sh $(TARGET)

clean:
	rm -rf $(BUILD_DIR) data/
