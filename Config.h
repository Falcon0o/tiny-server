#ifndef _CONFIG_H_INCLUDED_
#define _CONFIG_H_INCLUDED_

#include "LinuxConfig.h"
#include "Types.h"
#include "Setting.h"
#include "GlobalVal.h"
enum class LogLevel {
    debug = 0,
    info = 1,
    alert = 2,
    error = 3
};


#define LOG_ERROR(level, ...)               \
    fprintf(g_log_file, __VA_ARGS__);       \
    if (level >= LogLevel::alert) {         \
        fflush(g_log_file);                 \
    }

#define log_error(...)
#define log_error0(...)
// #define LOG_ERROR_0(level, log)  fprintf(g_log_file, log); 


extern const void *ERROR_ADDR_token;
#define ERROR_ADDR(type)  reinterpret_cast<type*>(const_cast<void*>(ERROR_ADDR_token))

extern const void *DECLINED_ADDR_token;
#define DECLINED_ADDR(type)  reinterpret_cast<type*>(const_cast<void*>(DECLINED_ADDR_token))

void debug_point();

#define     DECLINED    -5
#define     DONE        -4
#define     AGAIN       -2
#define     ERROR       -1
#define     OK          0

#define     PARSE_INVALID_VERSION       12
#define     HTTP_OK                     200
#define     CREATED                     201
#define     NO_CONTENT                  204
#define     PARTIAL_CONTENT             206
#define     SPECIAL_RESPONSE            300
#define     NOT_MODIFIED                304
#define     BAD_REQUEST                 400
#define     NOT_FOUND                   404
#define     NOT_ALLOWED                 405
#define     REQUEST_TIME_OUT            408
#define     REQUEST_URI_TOO_LARGE       414
/* Our own HTTP codes */
/* The special code to close connection without any response */
#define     CLOSE                       444
#define     REQUEST_HEADER_TOO_LARGE    494
/*
 * HTTP does not define the code for the case when a client closed
 * the connection while we are processing its request so we introduce
 * own code to log such situation when a client has closed the connection
 * before we even try to send the HTTP header to it
 */
#define     CLIENT_CLOSED_REQUEST       499

#define     INTERNAL_SERVER_ERROR       500
#define     VERSION_NOT_SUPPORTED       505

// void debug_point();

#define     LF     (u_char) '\n'
#define     CR     (u_char) '\r'
#define     CRLF   "\r\n"

size_t hash(size_t key, u_char c);
size_t hash_string(const char *c);
#endif