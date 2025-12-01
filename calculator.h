/**
 * @file calculator.h
 * @brief Calculator functions for parsing and evaluating mathematical expressions
 * @author skypenguins
 * @date 2025-12-02
 */

#ifndef CALCULATOR_H
#define CALCULATOR_H

#include <stdbool.h>

/**
 * @brief Validate query string for safe parsing
 * @param[in] query Query string to validate
 * @return true if valid, false otherwise
 */
bool validate_query(const char* query);

/**
 * @brief Extract and calculate mathematical expression from query string
 * @param[in] query Query string containing expression (e.g., "5+3", "10-2")
 * @param[out] result Pointer to store calculation result
 * @return true if calculation succeeded, false on error
 * @note Supports operators: +, -, *, /
 * @note Performs overflow checking for safe integer arithmetic
 */
bool calculate_query(const char* query, int* result);

#endif // CALCULATOR_H
