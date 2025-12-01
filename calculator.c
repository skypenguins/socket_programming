/**
 * @file calculator.c
 * @brief Calculator functions implementation
 * @author skypenguins
 * @date 2025-12-02
 */

#include "calculator.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

#define MAX_QUERY_LEN 256  /**< Maximum query parameter length */

bool validate_query(const char* query) {
    if (!query || strlen(query) > MAX_QUERY_LEN) {
        return false;
    }

    // Check for valid characters: digits, operators, whitespace
    for (const char* p = query; *p; p++) {
        if (!isdigit((unsigned char)*p) &&
            *p != '+' && *p != '-' && *p != '*' && *p != '/' &&
            *p != '=' && !isspace((unsigned char)*p)) {
            return false;
        }
    }

    return true;
}

bool calculate_query(const char* query, int* result) {
    if (!query || !result) {
        return false;
    }

    if (!validate_query(query)) {
        fprintf(stderr, "Invalid query string\n");
        return false;
    }

    int a = 0, b = 0;
    char op = '\0';

    // Skip leading '=' if present
    if (query[0] == '=') {
        query++;
    }

    int matched = sscanf(query, "%d%c%d", &a, &op, &b);
    if (matched != 3) {
        fprintf(stderr, "Failed to parse query: '%s'\n", query);
        return false;
    }

    // Perform calculation with overflow checking
    switch (op) {
        case '+':
            if ((b > 0 && a > INT_MAX - b) || (b < 0 && a < INT_MIN - b)) {
                fprintf(stderr, "Integer overflow in addition\n");
                return false;
            }
            *result = a + b;
            break;
        case '-':
            if ((b < 0 && a > INT_MAX + b) || (b > 0 && a < INT_MIN + b)) {
                fprintf(stderr, "Integer overflow in subtraction\n");
                return false;
            }
            *result = a - b;
            break;
        case '*':
            if (a != 0 && b != 0 && (a > INT_MAX / b || a < INT_MIN / b)) {
                fprintf(stderr, "Integer overflow in multiplication\n");
                return false;
            }
            *result = a * b;
            break;
        case '/':
            if (b == 0) {
                fprintf(stderr, "Division by zero\n");
                return false;
            }
            if (a == INT_MIN && b == -1) {
                fprintf(stderr, "Integer overflow in division\n");
                return false;
            }
            *result = a / b;
            break;
        default:
            fprintf(stderr, "Invalid operator: '%c'\n", op);
            return false;
    }

    return true;
}
