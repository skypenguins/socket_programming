# Makefile for socket programming projects
# Author: skypenguins
# Date: 2025-12-01

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c17 -pedantic -O2
CFLAGS += -D_POSIX_C_SOURCE=200809L -D_DARWIN_C_SOURCE
CFLAGS += -Wformat=2 -Wformat-security -Wstrict-prototypes
CFLAGS += -Wmissing-prototypes -Wshadow -Wpointer-arith
CFLAGS += -Wcast-qual -Wwrite-strings

# Debug flags (use with 'make DEBUG=1')
ifdef DEBUG
CFLAGS += -g -O0 -DDEBUG -fsanitize=address -fsanitize=undefined
LDFLAGS += -fsanitize=address -fsanitize=undefined
endif

# Targets
SERVER = http_server
CLIENT = http_client
TARGETS = $(SERVER) $(CLIENT)

# Source and object files
COMMON_SRC = http_utils.c calculator.c
SERVER_SRC = http_server.c
CLIENT_SRC = http_client.c
COMMON_OBJ = $(COMMON_SRC:.c=.o)
SERVER_OBJ = $(SERVER_SRC:.c=.o)
CLIENT_OBJ = $(CLIENT_SRC:.c=.o)
HEADERS = http_utils.h calculator.h

# Default target
all: $(TARGETS)

# Build server
$(SERVER): $(SERVER_OBJ) $(COMMON_OBJ)
	@echo "Linking $@..."
	$(CC) $(LDFLAGS) -o $@ $^
	@echo "Built $@ successfully"

# Build client
$(CLIENT): $(CLIENT_OBJ) $(COMMON_OBJ)
	@echo "Linking $@..."
	$(CC) $(LDFLAGS) -o $@ $^
	@echo "Built $@ successfully"

# Compile source files
%.o: %.c $(HEADERS)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	rm -f $(TARGETS) *.o *.dSYM
	rm -rf *.dSYM/
	@echo "Clean complete"

# Rebuild everything
rebuild: clean all

# Run server (requires root for port 80)
run-server: $(SERVER)
	@echo "Starting server (requires root for port 80)..."
	sudo ./$(SERVER)

# Run client
run-client: $(CLIENT)
	@echo "Running client..."
	./$(CLIENT)

# Format code (requires clang-format)
format:
	@if command -v clang-format >/dev/null 2>&1; then \
		echo "Formatting source files..."; \
		clang-format -i *.c *.h; \
		echo "Format complete"; \
	else \
		echo "clang-format not found, skipping format"; \
	fi

# Static analysis (requires clang-tidy)
analyze:
	@if command -v clang-tidy >/dev/null 2>&1; then \
		echo "Running static analysis..."; \
		clang-tidy $(COMMON_SRC) $(SERVER_SRC) $(CLIENT_SRC) -- $(CFLAGS); \
	else \
		echo "clang-tidy not found, skipping analysis"; \
	fi

# Help message
help:
	@echo "Available targets:"
	@echo "  all          - Build all targets (default)"
	@echo "  clean        - Remove build artifacts"
	@echo "  rebuild      - Clean and rebuild all"
	@echo "  run-server   - Build and run server (requires root)"
	@echo "  run-client   - Build and run client"
	@echo "  format       - Format source code (requires clang-format)"
	@echo "  analyze      - Run static analysis (requires clang-tidy)"
	@echo "  help         - Show this help message"
	@echo ""
	@echo "Debug build: make DEBUG=1"

.PHONY: all clean rebuild run-server run-client format analyze help
