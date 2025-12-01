/**
 * @file http_client.c
 * @brief Simple HTTP client implementation using TCP socket
 * @author skypenguins
 * @date 2025-12-01
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>

#define DEFAULT_MESSAGE "Hello, world!" /**< Default message to send */
#define DEFAULT_PORT "80"               /**< Default port number */
#define DEFAULT_SERVER "localhost"      /**< Default server address */
#define BUFFER_SIZE 8192                /**< Buffer size for communication */
#define MAX_MESSAGE_LEN 4096            /**< Maximum message length */

/**
 * @struct client_config_t
 * @brief Client configuration structure
 */
typedef struct {
    const char *server_name; /**< Server hostname or IP address */
    const char *port_name;   /**< Port number or service name */
    const char *message;     /**< Message to send to server */
} client_config_t;

/**
 * @brief Parse command line arguments
 * @param[in] argc Argument count
 * @param[in] argv Argument vector
 * @param[out] config Pointer to client configuration structure
 * @return true if arguments are valid, false otherwise
 */
static bool parse_arguments(int argc, char *argv[], client_config_t *config) {
    if (!config) {
        return false;
    }

    config->server_name = DEFAULT_SERVER;
    config->port_name = DEFAULT_PORT;
    config->message = DEFAULT_MESSAGE;

    if (argc >= 2) {
        config->server_name = argv[1];
        if (argc >= 3) {
            config->port_name = argv[2];
            if (argc >= 4) {
                config->message = argv[3];
                if (strlen(argv[3]) > MAX_MESSAGE_LEN) {
                    fprintf(stderr, "Error: Message too long (max %d bytes)\n", MAX_MESSAGE_LEN);
                    return false;
                }
            }
        }
    }

    return true;
}

/**
 * @brief Resolve hostname and service using getaddrinfo
 * @param[in] hostname Server hostname or IP address
 * @param[in] service Port number or service name (e.g., "http", "80")
 * @param[out] result Pointer to store address info list (caller must free with freeaddrinfo)
 * @return true if successful, false otherwise
 */
static bool resolve_address(const char *hostname, const char *service, struct addrinfo **result) {
    if (!hostname || !service || !result) {
        fprintf(stderr, "Error: Invalid arguments to resolve_address\n");
        return false;
    }

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;      // IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;  // TCP
    hints.ai_protocol = IPPROTO_TCP;

    int status = getaddrinfo(hostname, service, &hints, result);
    if (status != 0) {
        fprintf(stderr, "Error: getaddrinfo: %s\n", gai_strerror(status));
        return false;
    }

    return true;
}

/**
 * @brief Create socket and connect to server
 * @param[in] addr_info Address info structure from getaddrinfo
 * @return Socket file descriptor on success, -1 on failure
 */
static int connect_to_server(const struct addrinfo *addr_info) {
    if (!addr_info) {
        fprintf(stderr, "Error: Invalid address info\n");
        return -1;
    }

    int sockfd = socket(addr_info->ai_family, addr_info->ai_socktype, addr_info->ai_protocol);
    if (sockfd < 0) {
        perror("socket");
        return -1;
    }

    // Set socket timeout for better error handling
    struct timeval timeout;
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;

    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        perror("Warning: setsockopt SO_RCVTIMEO");
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0) {
        perror("Warning: setsockopt SO_SNDTIMEO");
    }

    if (connect(sockfd, addr_info->ai_addr, addr_info->ai_addrlen) < 0) {
        perror("connect");
        close(sockfd);
        return -1;
    }

    return sockfd;
}

/**
 * @brief Send complete message to server (handles partial writes)
 * @param[in] sockfd Socket file descriptor
 * @param[in] message Message string to send
 * @return true if successful, false otherwise
 */
static bool send_message(int sockfd, const char *message) {
    if (!message) {
        fprintf(stderr, "Error: Invalid message\n");
        return false;
    }

    size_t msg_len = strlen(message);
    size_t total_sent = 0;

    while (total_sent < msg_len) {
        ssize_t sent = write(sockfd, message + total_sent, msg_len - total_sent);
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
 * @brief Receive response from server
 * @param[in] sockfd Socket file descriptor
 * @param[out] response Buffer to store response
 * @param[in] response_size Size of response buffer
 * @return Number of bytes received on success, -1 on error, 0 on connection close
 */
static ssize_t receive_response(int sockfd, char *response, size_t response_size) {
    if (!response || response_size == 0) {
        fprintf(stderr, "Error: Invalid response buffer\n");
        return -1;
    }

    ssize_t nbytes = read(sockfd, response, response_size - 1);
    if (nbytes < 0) {
        if (errno == EINTR) {
            fprintf(stderr, "Warning: read interrupted by signal\n");
        } else {
            perror("read");
        }
        return -1;
    }

    if (nbytes == 0) {
        fprintf(stderr, "Connection closed by server\n");
        return 0;
    }

    response[nbytes] = '\0';
    return nbytes;
}

/**
 * @brief Main function
 * @param[in] argc Argument count
 * @param[in] argv Argument vector (server, port, message)
 * @return EXIT_SUCCESS on success, EXIT_FAILURE on failure
 */
int main(int argc, char *argv[]) {
    client_config_t config;
    struct addrinfo *addr_list = NULL;
    int sockfd = -1;
    int exit_code = EXIT_FAILURE;
    char buf[BUFFER_SIZE];

    // Parse command line arguments
    if (!parse_arguments(argc, argv, &config)) {
        fprintf(stderr, "Usage: %s [server] [port] [message]\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Resolve server address
    if (!resolve_address(config.server_name, config.port_name, &addr_list)) {
        goto cleanup;
    }

    // Try connecting to the resolved addresses
    for (struct addrinfo *addr = addr_list; addr != NULL; addr = addr->ai_next) {
        sockfd = connect_to_server(addr);
        if (sockfd >= 0) {
            break;  // Successfully connected
        }
    }

    if (sockfd < 0) {
        fprintf(stderr, "Error: Failed to connect to %s:%s\n",
                config.server_name, config.port_name);
        goto cleanup;
    }

    // Send message
    if (!send_message(sockfd, config.message)) {
        fprintf(stderr, "Error: Failed to send message\n");
        goto cleanup;
    }

    // Receive response
    ssize_t received = receive_response(sockfd, buf, sizeof(buf));
    if (received > 0) {
        printf("Received %zd bytes:\n%s\n", received, buf);
        exit_code = EXIT_SUCCESS;
    } else {
        fprintf(stderr, "Error: Failed to receive response\n");
    }

cleanup:
    if (sockfd >= 0) {
        close(sockfd);
    }
    if (addr_list != NULL) {
        freeaddrinfo(addr_list);
    }

    return exit_code;
}
