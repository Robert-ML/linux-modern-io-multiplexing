#ifndef CODE_ECHO_SERVER_C_SERVER_UTILS_H_
#define CODE_ECHO_SERVER_C_SERVER_UTILS_H_


#include <signal.h>
#include <stdint.h>


#define DEFAULT_SERVER_LISTENING_BACKLOG 128
#define BENCH_EXPECTED_MESSAGES 10000000

#ifndef SERVER_WARMUP_TIME_S
#define SERVER_WARMUP_TIME_S 10
#endif


extern volatile int server_running;

/**
 * @brief: Private Data file descriptor: this structure holds a file descriptor
 * with the purpose to store the listening socket of the server. Other similar
 * structures can hold a client's connection information, BUT they MUST have
 * the first field an int to the file descriptor they hold. This is to
 * differentiate between the exact struct a pointer points to; we check the
 * `fd` field if it is the server's listening socket.
 *
 * @example:
 * int listening_socket = 3;
 * pd_fd_t listening_socket_holder = { .fd = listening_socket };
 *
 * // another structure with THE FIRST 4 bytes the fd field
 * pd_other_struct_t con = { .fd = 7 };
 *
 * void *ambiguous_ptr;
 * ambiguous_ptr = (void *)&listening_socket_holder;
 * ambiguous_ptr = (void *)&con;
 *
 * if (((pd_fd_t *)ambiguous_ptr)->fd == listening_socket) {
 *     // this is the struct holding the listening socket
 *     pd_fd_t *data = (pd_fd_t *)ambiguous_ptr;
 * } else {
 *     // this is a connection holder
 *     pd_other_struct_t *data = (pd_other_struct_t *) ambiguous_ptr;
 * }
 */
typedef struct pd_fd {
    int fd;
} pd_fd_t;


/**
 * @brief: Create a timer usually used to signal the server warmup period
 * finished and can start benchmarking.
 */
int create_warmup_timer(void);


void register_signal_handler(void);
/**
 * @brief: Signal handler that modifies the global variable to stop the server.
 */
void stop_server_signal_handler(int signum);


/**
 * @brief: Creates a network socket on IPV4 standard in `SOCK_STREAM` and
 * `NONBLOCK` mode. Then binds it to the given `port` and allows connections
 * from any IP address (`INADDR_ANY`).
 *
 * @return: The socket file descriptor.
 *
 * @note: On error it crashes the program.
 */
int create_ipv4_listening_socket(const uint16_t port);


/**
 * @brief: Accepts a new client connection on the given listening socket.
 *
 * @return: The socket file descriptor for the new client connection.
 *
 * @note: On error it crashes the program.
 */
int accept_client(const int listening_socket);


/**
 * @brief: Reads from the fd into the buffer and echoes back immediately.
 *
 * @param fd: file descriptor to read from
 * @param buf: buffer to read data into and then out of (leaves the data in)
 * @param buffer_size: the size of the buffer
 *
 * @return: The number of bytes read into the buffer.
 *
 * @note: On error it crashed the program.
 * @note: For unblocking fds, EAGAIN errno is treated as an error.
 */
int read_n_echo(const int fd, uint8_t * const buf, const int buffer_size);

#endif /* CODE_ECHO_SERVER_C_SERVER_UTILS_H_ */
