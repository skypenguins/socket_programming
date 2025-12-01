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
#define DEFAULT_PORT "http"             /**< Default port (service name) */
#define DEFAULT_SERVER "localhost"      /**< Default server address */
#define BUFFER_SIZE BUFSIZ              /**< Buffer size for communication */

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
 */
void parse_arguments(int argc, char *argv[], client_config_t *config) {
    if (!config) {
        return;
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
            }
        }
    }
}

/**
 * @brief Resolve port number from service name
 * @param[in] portname Port name or service name (e.g., "http", "80")
 * @param[out] port Pointer to store resolved port number
 * @return true if successful, false otherwise
 */
bool resolve_port(const char *portname, uint16_t *port) {
    if (!portname || !port) {
        return false;
    }

    struct servent *serv = getservbyname(portname, "tcp");
    if (serv == NULL) {
        perror("getservbyname");
        return false;
    }
    *port = (uint16_t)serv->s_port;
    return true;
}

/**
 * @brief Resolve hostname to IP address
 * @param[in] servername Server hostname or IP address string
 * @param[out] addr Pointer to store resolved IP address
 * @return true if successful, false otherwise
 */
bool resolve_host(const char *servername, struct in_addr *addr) {
    if (!servername || !addr) {
        return false;
    }

    struct hostent *servhost = gethostbyname(servername);

    if (servhost == NULL) {
        in_addr_t ip_addr = inet_addr(servername);
        if (ip_addr == INADDR_NONE) {
            perror("gethostbyname");
            return false;
        }
        servhost = gethostbyaddr((const void *)&ip_addr, sizeof(ip_addr), AF_INET);
        if (servhost == NULL) {
            perror("gethostbyaddr");
            return false;
        }
    }

    memcpy(addr, servhost->h_addr, (size_t)servhost->h_length);
    return true;
}

/**
 * @brief Build server socket address structure
 * @param[out] servaddr Pointer to server address structure
 * @param[in] host_addr Pointer to resolved host address
 * @param[in] port Port number in network byte order
 */
void build_server_address(struct sockaddr_in *servaddr,
                          const struct in_addr *host_addr,
                          uint16_t port) {
    if (!servaddr || !host_addr) {
        return;
    }

    memset(servaddr, 0, sizeof(*servaddr));
    servaddr->sin_family = AF_INET;
    servaddr->sin_port = port;
    servaddr->sin_addr = *host_addr;
}

/**
 * @brief Create socket and connect to server
 * @param[in] servaddr Pointer to server address structure
 * @return Socket file descriptor on success, -1 on failure
 */
int connect_to_server(const struct sockaddr_in *servaddr) {
    if (!servaddr) {
        return -1;
    }

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return -1;
    }

    if (connect(sockfd, (const struct sockaddr *)servaddr, sizeof(*servaddr)) < 0) {
        perror("connect");
        close(sockfd);
        return -1;
    }

    return sockfd;
}

/**
 * @brief Send message to server and receive response
 * @param[in] sockfd Socket file descriptor
 * @param[in] message Message string to send
 * @param[out] response Buffer to store response
 * @param[in] response_size Size of response buffer
 * @return true if successful, false otherwise
 */
bool send_and_receive(int sockfd, const char *message, char *response, size_t response_size) {
    if (!message || !response || response_size == 0) {
        return false;
    }

    size_t msg_len = strlen(message) + 1;
    ssize_t nbytes = write(sockfd, message, msg_len);
    if (nbytes < 0) {
        perror("write");
        return false;
    }

    nbytes = read(sockfd, response, response_size - 1);
    if (nbytes < 0) {
        perror("read");
        return false;
    }

    if (nbytes == 0) {
        fprintf(stderr, "Connection closed by server\n");
        return false;
    }

    response[nbytes] = '\0';
    return true;
}

/**
 * @brief Main function
 * @param[in] argc Argument count
 * @param[in] argv Argument vector (server, port, message)
 * @return EXIT_SUCCESS on success, EXIT_FAILURE on failure
 */
int main(int argc, char *argv[]) {
    client_config_t config;
    struct sockaddr_in servaddr;
    struct in_addr host_addr;
    uint16_t port;
    char buf[BUFFER_SIZE];

    parse_arguments(argc, argv, &config);

    if (!resolve_port(config.port_name, &port)) {
        return EXIT_FAILURE;
    }

    if (!resolve_host(config.server_name, &host_addr)) {
        return EXIT_FAILURE;
    }

    build_server_address(&servaddr, &host_addr, port);

    int sockfd = connect_to_server(&servaddr);
    if (sockfd < 0) {
        return EXIT_FAILURE;
    }

    if (send_and_receive(sockfd, config.message, buf, sizeof(buf))) {
        puts(buf);
    }

    close(sockfd);
    return EXIT_SUCCESS;
}
