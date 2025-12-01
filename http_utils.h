/**
 * @file http_utils.h
 * @brief HTTP utility functions declarations
 * @author skypenguins
 * @date 2025-12-01
 */

#ifndef HTTP_UTILS_H
#define HTTP_UTILS_H

#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Decode URL-encoded string
 * @param[in] src Source URL-encoded string
 * @param[out] dst Destination buffer for decoded string
 * @param[in] dst_size Size of destination buffer
 * @return true if successful, false on error (buffer too small, invalid encoding)
 * @note Handles percent-encoding (e.g., %20 -> space)
 */
bool url_decode(const char* src, char* dst, size_t dst_size);

#endif // HTTP_UTILS_H
