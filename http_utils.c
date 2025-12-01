/**
 * @file http_utils.c
 * @brief HTTP utility functions implementation
 * @author skypenguins
 * @date 2025-12-01
 */

#include "http_utils.h"

#include <stdio.h>
#include <ctype.h>

bool url_decode(const char* src, char* dst, size_t dst_size) {
    if (!src || !dst || dst_size == 0) {
        return false;
    }

    const char* src_ptr = src;
    char* dst_ptr = dst;
    char* dst_end = dst + dst_size - 1;

    while (*src_ptr && dst_ptr < dst_end) {
        if (*src_ptr == '%') {
            // Validate hex digits are present
            if (!src_ptr[1] || !src_ptr[2]) {
                fprintf(stderr, "Invalid URL encoding: incomplete percent sequence\n");
                return false;
            }

            // Validate hex digits
            if (!isxdigit((unsigned char)src_ptr[1]) ||
                !isxdigit((unsigned char)src_ptr[2])) {
                fprintf(stderr, "Invalid URL encoding: non-hex character\n");
                return false;
            }

            unsigned int value = 0;
            if (sscanf(src_ptr + 1, "%2x", &value) == 1) {
                *dst_ptr++ = (char)value;
                src_ptr += 3;
            } else {
                return false;
            }
        } else if (*src_ptr == '+') {
            // '+' is used for space in query strings
            *dst_ptr++ = ' ';
            src_ptr++;
        } else {
            *dst_ptr++ = *src_ptr++;
        }
    }

    // Check if we ran out of buffer space
    if (*src_ptr != '\0') {
        fprintf(stderr, "URL decode buffer too small\n");
        return false;
    }

    *dst_ptr = '\0';
    return true;
}
