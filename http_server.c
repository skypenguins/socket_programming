/**
 * @file http_server.c
 * @brief Simple HTTP server with calculator functionality
 * @author skypenguins
 * @date 2025-12-01
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <limits.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>
#include <signal.h>

#include "http_utils.h"
#include "calculator.h"

#define SERVER_PORT 80              /**< HTTP server port */
#define LISTEN_BACKLOG 5            /**< Maximum pending connections */
#define BUFFER_SIZE 8192            /**< Buffer size for I/O operations */
#define MAX_REQUEST_SIZE 4096       /**< Maximum HTTP request size */
#define MAX_QUERY_LEN 256           /**< Maximum query parameter length */
#define RETRY_DELAY_SEC 1           /**< Retry delay in seconds on socket errors */
#define SOCKET_TIMEOUT_SEC 30       /**< Socket read/write timeout */

/**
 * @brief Extract query parameter from HTTP request
 * @param[in] request HTTP request string
 * @param[out] query Buffer to store extracted query parameter
 * @param[in] query_size Size of query buffer
 * @return true if query parameter found, false otherwise
 * @note Expects GET /calc?query=... format
 */
static bool extract_query_param(const char* request, char* query, size_t query_size) {
    if (!request || !query || query_size == 0) {
        return false;
    }

    const char prefix[] = "GET /calc?query=";
    const size_t prefix_len = sizeof(prefix) - 1;

    if (strncmp(request, prefix, prefix_len) != 0) {
        return false;
    }

    const char* query_start = request + prefix_len;

    // Find the end of the query (space or HTTP version marker)
    const char* query_end = strchr(query_start, ' ');
    if (!query_end) {
        query_end = strstr(query_start, "\r\n");
        if (!query_end) {
            fprintf(stderr, "Malformed HTTP request\n");
            return false;
        }
    }

    size_t len = (size_t)(query_end - query_start);
    if (len >= query_size) {
        fprintf(stderr, "Query parameter too long (max %zu bytes)\n", query_size - 1);
        return false;
    }

    if (len == 0) {
        fprintf(stderr, "Empty query parameter\n");
        return false;
    }

    memcpy(query, query_start, len);
    query[len] = '\0';

    return true;
}

/**
 * @brief Send HTTP response with status code and optional body
 * @param[in] connfd Connected socket file descriptor
 * @param[in] status_code HTTP status code (e.g., 200, 400, 500)
 * @param[in] body Response body (NULL for empty body)
 * @return true if successful, false otherwise
 */
static bool send_http_response(int connfd, int status_code, const char* body) {
    char response[BUFFER_SIZE];
    const char* status_text;

    // Map status code to text
    switch (status_code) {
        case 200: status_text = "OK"; break;
        case 400: status_text = "Bad Request"; break;
        case 500: status_text = "Internal Server Error"; break;
        default: status_text = "Unknown"; break;
    }

    const char* response_body = body ? body : "";
    size_t body_len = strlen(response_body);

    int len = snprintf(response, sizeof(response),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n"
        "\r\n"
        "%s",
        status_code, status_text, body_len, response_body);

    if (len < 0 || (size_t)len >= sizeof(response)) {
        fprintf(stderr, "Failed to format HTTP response\n");
        return false;
    }

    // Send complete response (handle partial writes)
    size_t total_sent = 0;
    size_t response_len = (size_t)len;

    while (total_sent < response_len) {
        ssize_t sent = write(connfd, response + total_sent, response_len - total_sent);
        if (sent < 0) {
            if (errno == EINTR) {
                continue;  // Interrupted by signal, retry
            }
            perror("write");
            return false;
        }
        total_sent += (size_t)sent;
    }

    return true;
}

/**
 * @brief Handle incoming HTTP request
 * @param[in] connfd Connected socket file descriptor
 * @note Reads request, extracts query, decodes it, calculates result, and sends response
 */
static void handle_request(int connfd) {
    char buf[MAX_REQUEST_SIZE];
    char raw_query[MAX_QUERY_LEN];
    char decoded_query[MAX_QUERY_LEN];
    char response_body[64];
    int calc_result = 0;

    // Set socket timeout
    struct timeval timeout;
    timeout.tv_sec = SOCKET_TIMEOUT_SEC;
    timeout.tv_usec = 0;
    if (setsockopt(connfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        perror("Warning: setsockopt SO_RCVTIMEO");
    }

    // Read request
    ssize_t nbytes = read(connfd, buf, sizeof(buf) - 1);
    if (nbytes < 0) {
        perror("read");
        send_http_response(connfd, 500, "Internal Server Error");
        return;
    }

    if (nbytes == 0) {
        return;  // Connection closed
    }

    buf[nbytes] = '\0';

    // Extract query parameter
    if (!extract_query_param(buf, raw_query, sizeof(raw_query))) {
        send_http_response(connfd, 400, "Bad Request: Invalid query format");
        return;
    }

    // URL decode
    if (!url_decode(raw_query, decoded_query, sizeof(decoded_query))) {
        send_http_response(connfd, 400, "Bad Request: Invalid URL encoding");
        return;
    }

    printf("Query(raw): %s\n", raw_query);
    printf("Query(decoded): %s\n", decoded_query);

    // Calculate result
    if (!calculate_query(decoded_query, &calc_result)) {
        send_http_response(connfd, 400, "Bad Request: Invalid calculation");
        return;
    }

    printf("Result: %d\n", calc_result);

    // Format and send response
    snprintf(response_body, sizeof(response_body), "%d", calc_result);
    send_http_response(connfd, 200, response_body);
}

/**
 * @brief Create and initialize server socket
 * @return Server socket file descriptor on success, -1 on fatal error
 * @note Sets SO_REUSEADDR option to allow quick restart
 * @note Binds to all interfaces (in6addr_any) on SERVER_PORT
 * @note Supports dual-stack (IPv4 and IPv6) by disabling IPV6_V6ONLY
 */
static int create_server_socket(void) {
    int listenfd;
    struct sockaddr_in6 servaddr;
    int retry_count = 0;
    const int max_retries = 3;

    while (retry_count < max_retries) {
        listenfd = socket(AF_INET6, SOCK_STREAM, 0);
        if (listenfd < 0) {
            perror("socket");
            retry_count++;
            if (retry_count < max_retries) {
                fprintf(stderr, "Retrying (%d/%d)...\n", retry_count, max_retries);
                sleep(RETRY_DELAY_SEC);
            }
            continue;
        }

        // Enable address reuse to avoid "Address already in use" errors
        int opt = 1;
        if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
            perror("setsockopt SO_REUSEADDR");
            close(listenfd);
            retry_count++;
            if (retry_count < max_retries) {
                fprintf(stderr, "Retrying (%d/%d)...\n", retry_count, max_retries);
                sleep(RETRY_DELAY_SEC);
            }
            continue;
        }

        // Disable IPV6_V6ONLY to enable dual-stack (accept both IPv4 and IPv6)
        int v6only = 0;
        if (setsockopt(listenfd, IPPROTO_IPV6, IPV6_V6ONLY, &v6only, sizeof(v6only)) < 0) {
            perror("setsockopt IPV6_V6ONLY");
            close(listenfd);
            retry_count++;
            if (retry_count < max_retries) {
                fprintf(stderr, "Retrying (%d/%d)...\n", retry_count, max_retries);
                sleep(RETRY_DELAY_SEC);
            }
            continue;
        }

        // Build server address structure
        memset(&servaddr, 0, sizeof(servaddr));
        servaddr.sin6_family = AF_INET6;
        servaddr.sin6_port = htons(SERVER_PORT);
        servaddr.sin6_addr = in6addr_any;

        // Bind socket to address
        if (bind(listenfd, (const struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
            perror("bind");
            close(listenfd);
            retry_count++;
            if (retry_count < max_retries) {
                fprintf(stderr, "Retrying (%d/%d)...\n", retry_count, max_retries);
                sleep(RETRY_DELAY_SEC);
            }
            continue;
        }

        // Start listening for connections
        if (listen(listenfd, LISTEN_BACKLOG) < 0) {
            perror("listen");
            close(listenfd);
            retry_count++;
            if (retry_count < max_retries) {
                fprintf(stderr, "Retrying (%d/%d)...\n", retry_count, max_retries);
                sleep(RETRY_DELAY_SEC);
            }
            continue;
        }

        return listenfd;
    }

    fprintf(stderr, "Failed to create server socket after %d retries\n", max_retries);
    return -1;
}

// Global flag for graceful shutdown
static volatile sig_atomic_t server_running = 1;

/**
 * @brief Signal handler for graceful shutdown
 * @param[in] signum Signal number
 */
static void signal_handler(int signum) {
    if (signum == SIGINT || signum == SIGTERM) {
        printf("\nReceived signal %d, shutting down gracefully...\n", signum);
        server_running = 0;
    }
}

/**
 * @brief Main server event loop
 * @param[in] listenfd Listening socket file descriptor
 * @note Accepts connections in a loop and handles each request
 * @note Supports graceful shutdown via SIGINT/SIGTERM
 * @note Handles both IPv4 and IPv6 client connections
 */
static void run_server(int listenfd) {
    struct sockaddr_storage client_addr;
    socklen_t client_len;

    while (server_running) {
        client_len = sizeof(client_addr);
        int connfd = accept(listenfd, (struct sockaddr*)&client_addr, &client_len);

        if (connfd < 0) {
            if (errno == EINTR) {
                // Interrupted by signal, check if we should continue
                continue;
            }
            perror("accept");
            continue;
        }

        // Log client connection (handle both IPv4 and IPv6)
        char client_ip[INET6_ADDRSTRLEN];
        uint16_t client_port;

        if (client_addr.ss_family == AF_INET) {
            // IPv4
            struct sockaddr_in *addr_in = (struct sockaddr_in*)&client_addr;
            inet_ntop(AF_INET, &addr_in->sin_addr, client_ip, sizeof(client_ip));
            client_port = ntohs(addr_in->sin_port);
        } else if (client_addr.ss_family == AF_INET6) {
            // IPv6
            struct sockaddr_in6 *addr_in6 = (struct sockaddr_in6*)&client_addr;
            inet_ntop(AF_INET6, &addr_in6->sin6_addr, client_ip, sizeof(client_ip));
            client_port = ntohs(addr_in6->sin6_port);
        } else {
            snprintf(client_ip, sizeof(client_ip), "unknown");
            client_port = 0;
        }

        printf("Connection from %s:%u\n", client_ip, client_port);

        handle_request(connfd);
        close(connfd);
    }
}

/**
 * @brief Main function
 * @return EXIT_SUCCESS on normal termination, EXIT_FAILURE on error
 * @note Creates server socket, sets up signal handlers, and starts event loop
 */
int main(void) {
    // Set up signal handlers for graceful shutdown
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGINT, &sa, NULL) < 0) {
        perror("sigaction SIGINT");
        return EXIT_FAILURE;
    }

    if (sigaction(SIGTERM, &sa, NULL) < 0) {
        perror("sigaction SIGTERM");
        return EXIT_FAILURE;
    }

    // Ignore SIGPIPE to prevent server crash when client disconnects
    signal(SIGPIPE, SIG_IGN);

    // Create and configure server socket
    int listenfd = create_server_socket();
    if (listenfd < 0) {
        fprintf(stderr, "Failed to create server socket\n");
        return EXIT_FAILURE;
    }

    printf("Server listening on port %d\n", SERVER_PORT);
    printf("Press Ctrl+C to stop the server\n");

    // Run server event loop
    run_server(listenfd);

    // Cleanup
    close(listenfd);
    printf("Server shutdown complete\n");
    return EXIT_SUCCESS;
}
