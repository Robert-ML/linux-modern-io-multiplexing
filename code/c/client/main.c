#include <arpa/inet.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>


#include "../common/debug.h"
#include "../common/defines.h"
#include "../common/utils.h"
#include "./defines.h"



char buffer[DEFAULT_BUFFER_SIZE];


static int create_ipv4_socket();
static void connect_to_server(const int socket_fd);

static void send_message(const int socket_fd);
static void recv_message(const int socket_fd);
static void send_n_wait(const int socket_fd);

/**
 * @brief: Loop to send messages `count` times. If count < 0 runs indefinitely.
 */
static void loop(const int socket_fd, int64_t count);


int main(int argc, char **argv) {
    int socket_fd;
    int64_t count;

    socket_fd = create_ipv4_socket();
    connect_to_server(socket_fd);

    dlog(LOG_DEBUG, "Client connected to server\n");

    if (argc == 1) {
        count = -1;
    } else {
        errno = 0;
        count = strtoll(argv[1], NULL, 10);
        assert_zero(errno, "stroll");
    }
    loop(socket_fd, count);

    close(socket_fd);

    return 0;
}


static void loop(const int socket_fd, int64_t count)
{
    while (count) {
#ifdef REQUEST_RESPONSE_MODE
        send_n_wait(socket_fd);
#elif FIREHOSE_MODE
        send_message(socket_fd);
#elif IDLE_MODE
#endif

        // controls the stop condition
        if (count < 0) {
            continue;
        }
        --count;
    }
}


static int create_ipv4_socket()
{
    int socket_fd;

    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    assert_nonn(socket_fd, "socket");

    return socket_fd;
}

static void connect_to_server(const int socket_fd)
{
    int rc;
    struct sockaddr_in serv_addr;

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);

    rc = inet_pton(AF_INET, SERVER_ADDRESS, &serv_addr.sin_addr);
    assert_nonn(rc - 1, "inet_pton"); // inet_pton returns -1 and 0 on fail

    rc = connect(socket_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    assert_nonn(rc, "connect");
}

static void send_n_wait(const int socket_fd)
{
    send_message(socket_fd);
    recv_message(socket_fd);
}

static void send_message(const int socket_fd)
{
    int rc;
    const int buffer_size = sizeof(buffer);

    rc = write(socket_fd, buffer, buffer_size);
    assert_nonn(rc, "write");
    if (rc < buffer_size) {
        dlog(LOG_CRIT, "write didn't finish in one call: %d bytes\n", rc);
    }
}

static void recv_message(const int socket_fd)
{
    int rc;
    const int buffer_size = sizeof(buffer);

    rc = read(socket_fd, buffer, buffer_size);
    assert_nonn(rc, "read");
    if (rc != buffer_size) {
        dlog(LOG_CRIT, "we lost data on read: expected %d bytes, got %d bytes\n", buffer_size, rc);
    }
}
