# AGENTS.md - Socket Programming Project Guide

## Project Overview
This is a C17-based socket programming project implementing HTTP client/server applications with calculator functionality. The project focuses on security, portability, and proper networking practices.

## Build & Development Commands

### Build Commands
```bash
# Standard build
make

# Debug build with AddressSanitizer
make DEBUG=1

# Clean build
make clean

# Rebuild everything
make rebuild

# Build specific targets
make http_server
make http_client
```

### Execution Commands
```bash
# Run server (requires root for port 80)
make run-server
sudo ./http_server

# Run client
make run-client
./http_client

# Direct execution
./http_client localhost http "GET /calc?query=2%2b11 HTTP/1.1"
```

### Code Quality Commands
```bash
# Format code (requires clang-format)
make format

# Static analysis (requires clang-tidy)
make analyze

# Show help
make help
```

## Code Style Guidelines

### Language & Standards
- **Language**: C17 with POSIX compliance
- **Compiler**: GCC with strict warning flags
- **Standard**: `-std=c17`
- **Platform**: POSIX 200809L + Darwin extensions

### Code Structure
**File Organization:**
- Each module has corresponding header files with proper include guards
- Public interfaces declared in headers, implementations in .c files
- Doxygen-style documentation comments for all public functions

**Import Guidelines:**
```c
// Standard library headers first
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// System headers
#include <sys/socket.h>
#include <netinet/in.h>

// Local headers (always quote)
#include "http_utils.h"
#include "calculator.h"
```

### Formatting Conventions
- **Indentation**: Tabs (not spaces)
- **Brace Style**: K&R style - opening brace on same line
- **Line Length**: Maximum 80 characters
- **Function Style**: 
```c
static bool function_name(const char* param, int* result) {
    if (!param || !result) {
        return false;
    }
    // implementation
    return true;
}
```

### Naming Conventions
- **Variables**: `snake_case` with descriptive names
- **Functions**: `snake_case` with action-oriented names
- **Constants**: `UPPER_SNAKE_CASE` with descriptive names
- **Typedefs**: `PascalCase` (e.g., `CalculatorResult`)

### Security & Error Handling
**Error Handling Pattern:**
```c
if (!validate_query(query)) {
    log_error("Invalid query string");
    return false;
}
```

**Security Requirements:**
- All external input must be validated before processing
- Buffer operations must respect size limits
- Integer overflow protection required
- Use `strncpy()` instead of `strcpy()` when size matters
- Never trust client-provided data without validation

**Memory Safety:**
- Check allocations for NULL returns
- Clean up resources in error paths
- Use `calloc()` for zero-initialized memory when needed

### HTTP Protocol Implementation
**Request Processing:**
- Support GET requests with query parameters
- Proper URL decoding with validation
- Content-Length header handling
- Graceful handling of malformed requests

**Response Guidelines:**
- Appropriate HTTP status codes (200, 400, 500)
- Content-Type: text/plain for calculator results
- Proper HTTP/1.1 protocol compliance

### Testing & Validation
**Current Testing Approach:**
- Manual testing expected (no automated tests)
- Test calculator functionality with various expressions
- Test HTTP client/server communication
- Test error conditions and edge cases

**Security Testing Points:**
- Buffer overflow attempts
- Integer overflow edge cases
- Malformed URL encoding
- Long query strings
- Division by zero scenarios

### Build Configuration
**Compiler Flags Explained:**
- `-Wall -Wextra -Werror`: Treat all warnings as errors
- `-Wformat=2`: Format string checking
- `-Wstrict-prototypes`: Prototype warnings
- `-Wmissing-prototypes`: Missing function prototypes
- `-Wshadow`: Variable shadowing detection
- `-Wpointer-arith`: Pointer arithmetic checks
- `-Wcast-qual`: Type qualifier warnings
- `-Wwrite-strings`: String literal checks

**Debug Configuration:**
```bash
# Debug build enables:
# - AddressSanitizer for memory errors
# - UndefinedBehaviorSanitizer for undefined behavior
# - Debug symbols (-g)
# - No optimization (-O0)
make DEBUG=1
```

### Project Structure
```
socket_programming/
├── http_server.c      # Main HTTP server implementation
├── http_client.c      # HTTP client implementation
├── calculator.c       # Mathematical expression evaluation
├── calculator.h       # Calculator function declarations
├── http_utils.c       # HTTP utility functions
├── http_utils.h       # HTTP utility declarations
├── Makefile           # Build system
└── README.md          # Project documentation
```

### Key Implementation Notes
1. **Dual-stack Support**: Both IPv4 and IPv6 implemented using `getaddrinfo()`
2. **Graceful Shutdown**: Proper signal handling for SIGINT/SIGTERM
3. **SIG Protection**: Protection against broken pipe errors
4. **Timeout Handling**: Socket timeouts to prevent hangs
5. **Portability**: POSIX with Darwin extensions for macOS compatibility

### Environment Setup
Required tools:
- GCC compiler
- sudo (for server on port 80)
- make

Optional tools for enhanced development:
- clang-format (for code formatting)
- clang-tidy (for static analysis)
- valgrind (for memory debugging)

### Development Workflow
1. Write code following style guidelines
2. Build with `make` to check for compilation errors
3. Test manually using curl or custom client
4. Run debug build for memory safety checks
5. Use static analysis tools when available