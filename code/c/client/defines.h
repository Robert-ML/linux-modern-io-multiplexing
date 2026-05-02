#ifndef CODE_C_CLIENT_DEFINES_H_
#define CODE_C_CLIENT_DEFINES_H_


#ifndef SERVER_ADDRESS
#define SERVER_ADDRESS "127.0.0.1"
#endif


#ifndef CLIENT_MODE
    #error "No operating mode specified for client"
#else
    #if CLIENT_MODE == 0
        #define REQUEST_RESPONSE_MODE
    #elif CLIENT_MODE == 1
        #define FIREHOSE_MODE
    #elif CLIENT_MODE == 3
        #define IDLE_MODE
    #else
        #error "Unknown operating mode specified for client"
    #endif /* CLIENT_MODE == 0 */
#endif /* CLIENT_MODE */

#endif /* CODE_C_CLIENT_DEFINES_H_ */
