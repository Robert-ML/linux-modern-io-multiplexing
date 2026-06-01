#define _GNU_SOURCE /* needed by accept4 */
#include "./server_utils.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/timerfd.h>
#include <sys/un.h>

#include "../common/utils.h"


volatile int server_running = 1;

int create_warmup_timer(void)
{
    int rc;
    struct itimerspec settings;
    const int timer_fd = timerfd_create(CLOCK_MONOTONIC, 0);

    assert_nonn(timer_fd, "timerfd_create");

    // set timer
    memset(&settings, 0, sizeof(settings));
    settings.it_value.tv_sec = SERVER_WARMUP_TIME_S;

    rc = timerfd_settime(timer_fd, 0, &settings, NULL);
    assert_zero(rc, "timerfd_settime");

    return timer_fd;
}

void register_signal_handler(void)
{
    int rc;
    struct sigaction act;
    memset(&act, 0, sizeof(act));

    act.sa_handler = stop_server_signal_handler;

    rc = sigaction(SIGUSR1, &act, NULL);
    assert_zero(rc, "sigaction");
}

void stop_server_signal_handler(int)
{
    server_running = 0;
}


int create_ipv4_listening_socket(const uint16_t port)
{
    int rc;
    int socket_fd;
    struct sockaddr_in my_addr;

    // create socket
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    assert_nonn(socket_fd, "socket");

    // make it non-blocking
    rc = fcntl(socket_fd, F_SETFL, O_NONBLOCK);
    assert_nonn(rc, "fcntl");

    // prepare binding information
    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(port);
    my_addr.sin_addr.s_addr = INADDR_ANY;

    // bind the socket
    rc = bind(socket_fd, (struct sockaddr *) &my_addr, sizeof(my_addr));
    assert_zero(rc, "bind");

    // listen on the socket
    rc = listen(socket_fd, DEFAULT_SERVER_LISTENING_BACKLOG);
    assert_zero(rc, "listen");

    return socket_fd;
}


int accept_client(const int listening_socket)
{
	int client_socket;
	struct sockaddr_in addr;
	socklen_t addrlen = sizeof(addr);

    client_socket = accept4(
        listening_socket,
        (struct sockaddr *) &addr,
        &addrlen,
        SOCK_NONBLOCK
    );
    if (client_socket < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        return -1;
    }
    assert_nonn(client_socket, "accept");

    return client_socket;
}

/**
 * @note: To not have fake message segmentation, the buffer should always be
 * bigger than the message to be received. I.e. the `read` and `write`
 * operations must finish in one call.
 */
int read_n_echo(const int fd, uint8_t * const buf, const int buffer_size)
{
    int read_bytes, sent_bytes;

    read_bytes = read(fd, buf, buffer_size);
    assert_nonn(read_bytes, "read\n");
    if (read_bytes == 0) {
        // close client
        return 0;
    } else if (read_bytes > (int) DEFAULT_BUFFER_SIZE) {
        dlog(LOG_CRIT, "recv: got more than expected");
        exit(EXIT_FAILURE);
    } else if (read_bytes < (int) DEFAULT_BUFFER_SIZE) {
        dlog(LOG_CRIT, "recv: got less than expected");
        exit(EXIT_FAILURE);
    }

    // echo back the message
    sent_bytes = write(fd, buf, read_bytes);
    assert_nonn(sent_bytes, "write\n");
    if (sent_bytes < (int) DEFAULT_BUFFER_SIZE) {
        dlog(LOG_CRIT, "send: less than expected");
        exit(EXIT_FAILURE);
    }

    return read_bytes;
}
