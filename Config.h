#ifndef _CONFIG_H_INCLUDED_
#define _CONFIG_H_INCLUDED_

#include <assert.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <string.h>

#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <sys/signal.h>
#include <sys/time.h>
#include <sys/types.h>

#include <time.h>

#include <unistd.h>
#include <stdint.h>

#include <list>
#include <set>
#include <map>
#include <vector>
#include <functional>


#include "Setting.h"

using Int       = intptr_t;
using uInt      = uintptr_t;

using mSec      = uInt;

extern class Cycle      *g_cycle;
extern class Process    *g_process;
extern class Epoller    *g_epoller;
extern Int               g_worker_id;
extern int               g_pagesize;

extern mSec              g_curr_msec;
extern class Timer      *g_timer;

#define log_error(level, log, ...)      printf(log, __VA_ARGS__); fflush(stdout);
#define log_error0(level, log)          printf(log); fflush(stdout);


#define     UNSET       0
#define     SET         1
#define     DISABLE     2

#define     DECLINED    -5
#define     DONE        -4
#define     AGAIN       -2
#define     ERROR       -1
#define     OK          0

#define     PARSE_INVALID_VERSION       12
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
/*
 * HTTP does not define the code for the case when a client closed
 * the connection while we are processing its request so we introduce
 * own code to log such situation when a client has closed the connection
 * before we even try to send the HTTP header to it
 */
#define     CLIENT_CLOSED_REQUEST       499

#define     INTERNAL_SERVER_ERROR       500
#define     VERSION_NOT_SUPPORTED       505

#endif