/**
 * @file http_server.c
 * @brief Simple HTTP server with calculator functionality
 * @author skypenguins
 * @date 2025-12-01
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <stdbool.h>

#define SERVER_PORT 80          /**< HTTP server port */
#define LISTEN_BACKLOG 5        /**< Maximum pending connections */
#define BUFFER_SIZE BUFSIZ      /**< Buffer size for I/O operations */
#define RETRY_DELAY_SEC 1       /**< Retry delay in seconds on socket errors */

/**
 * @brief Extract and calculate mathematical expression from query string
 * @param[in] query Query string containing expression (e.g., "5+3", "10-2")
 * @return Calculated result, or 0 if parsing fails
 * @note Supports operators: +, -, *, /
 */
int calculate_query(const char* query) {
    int a, b;
    char op;

    if(query[0] == '=') {
        query++;
    }

    printf("DEBUG calculate_query input: '%s'\n", query);
    int matched = sscanf(query, "%d%c%d", &a, &op, &b);
    printf("DEBUG sscanf matched: %d(a=%d, op=%c, b=%d)\n", matched, a, op, b);

    if(matched != 3) {
        return 0;
    }

    switch(op) {
        case '+': return a + b;
        case '-': return a - b;
        case '*': return a * b;
        case '/': return b != 0 ? a / b : 0;
        default: return 0;
    }
}

/**
 * @brief Decode URL-encoded string
 * @param[in] src Source URL-encoded string
 * @param[out] dst Destination buffer for decoded string
 * @param[in] dst_size Size of destination buffer
 * @note Handles percent-encoding (e.g., %20 -> space)
 */
void url_decode(const char* src, char* dst, size_t dst_size) {
    const char* src_ptr = src;
    char* dst_ptr = dst;
    char* dst_end = dst + dst_size - 1;

    while(*src_ptr && dst_ptr < dst_end) {
        if(*src_ptr == '%' && *(src_ptr + 1) && *(src_ptr + 2)) {
            int value;
            sscanf(src_ptr + 1, "%2x", &value);
            *dst_ptr++ = value;
            src_ptr += 3;
        } else {
            *dst_ptr++ = *src_ptr++;
        }
    }
    *dst_ptr = '\0';
}

/**
 * @brief Extract query parameter from HTTP request
 * @param[in] request HTTP request string
 * @param[out] query Buffer to store extracted query parameter
 * @param[in] query_size Size of query buffer
 * @return true if query parameter found, false otherwise
 * @note Expects GET /calc?query=... format
 */
bool extract_query_param(const char* request, char* query, size_t query_size) {
    if(strncmp(request, "GET /calc?query=", 16) != 0) {
        return false;
    }

    const char* query_start = request + 16;
    if(strncmp(query_start, "query=", 6) == 0) {
        query_start += 6;
    }

    const char* query_end = strchr(query_start, ' ');
    if(!query_end) {
        return false;
    }

    size_t len = query_end - query_start;
    if(len >= query_size) {
        len = query_size - 1;
    }

    strncpy(query, query_start, len);
    query[len] = '\0';

    return true;
}

/**
 * @brief Send HTTP response with calculation result
 * @param[in] connfd Connected socket file descriptor
 * @param[in] result Calculation result to send
 * @note Sends HTTP/1.1 200 OK response with Content-Length header
 */
void send_http_response(int connfd, int result) {
    char response[BUFFER_SIZE];
    char result_str[16];

    snprintf(result_str, sizeof(result_str), "%d", result);
    snprintf(response, sizeof(response),
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: %zu\r\n"
        "\r\n"
        "%s",
        strlen(result_str), result_str);

    if(write(connfd, response, strlen(response)) < 0) {
        perror("write");
    }
}

/**
 * @brief Handle incoming HTTP request
 * @param[in] connfd Connected socket file descriptor
 * @note Reads request, extracts query, decodes it, calculates result, and sends response
 */
void handle_request(int connfd) {
    char buf[BUFFER_SIZE];
    char raw_query[BUFFER_SIZE];
    char decoded_query[BUFFER_SIZE];

    ssize_t nbytes = read(connfd, buf, sizeof(buf) - 1);
    if(nbytes < 0) {
        perror("read");
        return;
    }

    if(nbytes == 0) {
        return;
    }

    buf[nbytes] = '\0';

    if(!extract_query_param(buf, raw_query, sizeof(raw_query))) {
        return;
    }

    url_decode(raw_query, decoded_query, sizeof(decoded_query));

    printf("Query(raw): %s\n", raw_query);
    printf("Query(decoded): %s\n", decoded_query);

    int result = calculate_query(decoded_query);
    printf("Result: %d\n", result);

    send_http_response(connfd, result);
}

/**
 * @brief Create and initialize server socket
 * @return Server socket file descriptor
 * @note Sets SO_REUSEADDR option and retries on failure with 1 second delay
 * @note Binds to all interfaces (INADDR_ANY) on SERVER_PORT
 */
int create_server_socket(void) {
    int listenfd;
    struct sockaddr_in servaddr;

    while(1) {
        listenfd = socket(AF_INET, SOCK_STREAM, 0);
        if(listenfd < 0) {
            perror("socket");
            sleep(RETRY_DELAY_SEC);
            continue;
        }

        int opt = 1;
        if(setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
            perror("setsockopt");
            close(listenfd);
            sleep(RETRY_DELAY_SEC);
            continue;
        }

        bzero(&servaddr, sizeof(servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_port = htons(SERVER_PORT);
        servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

        if(bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
            perror("bind");
            close(listenfd);
            sleep(RETRY_DELAY_SEC);
            continue;
        }

        if(listen(listenfd, LISTEN_BACKLOG) < 0) {
            perror("listen");
            close(listenfd);
            sleep(RETRY_DELAY_SEC);
            continue;
        }

        return listenfd;
    }
}

/**
 * @brief Main server event loop
 * @param[in] listenfd Listening socket file descriptor
 * @note Accepts connections in an infinite loop and handles each request
 */
void run_server(int listenfd) {
    while(1) {
        int connfd = accept(listenfd, (struct sockaddr*)NULL, NULL);
        if(connfd < 0) {
            perror("accept");
            continue;
        }

        handle_request(connfd);
        close(connfd);
    }
}

/**
 * @brief Main function
 * @return EXIT_SUCCESS on normal termination
 * @note Creates server socket and starts event loop
 */
int main(void) {
    int listenfd = create_server_socket();
    printf("Server listening on port %d\n", SERVER_PORT);
    run_server(listenfd);
    close(listenfd);
    return EXIT_SUCCESS;
}
