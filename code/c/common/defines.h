#ifndef CODE_C_COMMON_DEFINES_H_
#define CODE_C_COMMON_DEFINES_H_


// the listening port of the server
#ifndef SERVER_PORT
#define SERVER_PORT 8080U
#endif


// buffer size used to move data around
#ifndef DEFAULT_BUFFER_SIZE
#error "DEFAULT_BUFFER_SIZE undefined"
#endif
// server's buffers are a little bigger to detect if messages get segmented
#define DEFAULT_SERVER_BUFFER_SIZE DEFAULT_BUFFER_SIZE + sizeof(void *)


#endif /* CODE_C_COMMON_DEFINES_H_ */
