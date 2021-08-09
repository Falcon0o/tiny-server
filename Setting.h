#ifndef _SETTING_H_INCLUDED_
#define _SETTING_H_INCLUDED_

#include <sys/types.h>
#include <stdint.h>

#define MAX_TIME_T_VALUE    9223372036854775807LL
#define MAX_OFF_T_LEN       (sizeof("-9223372036854775808") - 1)
#define MAX_OFF_T_VALUE     9223372036854775807LL
#define MAX_TIME_T_LEN      (sizeof("-9223372036854775808") - 1)
#define MAX_SENDFILE_SIZE   2147483647L
#define IOVS_PREALLOCATE    64

using Int       = intptr_t;
using uInt      = uintptr_t;
using mSec      = uInt;

// enum {
//     UNSET   =   0,
//     SET     =   1,
//     DISABLE =   2
// };

constexpr size_t WORKER_CONNECTIONS = 512;

/* 最大 worker 进程数，为 0 时不做限制 */
constexpr size_t MAX_WORKER_PROCESSES = 1;
// constexpr unsigned int CLIENT_HEADER_BUFFER_SIZE = 100;
constexpr size_t CLIENT_HEADER_BUFFER_SIZE = 1024;

constexpr size_t LARGE_CLIENT_HEADER_BUFFER_NUM = 4;
// constexpr unsigned int LARGE_CLIENT_HEADER_BUFFER_SIZE = 101;
constexpr size_t LARGE_CLIENT_HEADER_BUFFER_SIZE = 8192;
constexpr size_t SENDFILE_MAX_CHUNK = 10 * 1024 * 1024;
constexpr size_t HTTP_LOWCAST_HEADER_LEN = 32;   /* 必须是2的整数倍 */

// 使用 MAX_OFF_T_VALUE关闭以下参数 此参数也会影响sendfile
constexpr off_t MIN_READ_AHEAD_SIZE = 128 * 1024;


constexpr bool IGNORE_INVALID_HEADERS = true;
constexpr bool ALLOW_UNDERSCORES_IN_HEADERS = true;

constexpr char PREFIX[] = "./resource";
constexpr char DEFAULT_CONTEXT_TYPE[] = "text/plain";

constexpr size_t TIME_SLOTS = 64;

/* 是否对超时的连接发送 RST */
constexpr bool RESET_TIMEDOUT_CONNECTION = true;
constexpr bool DAEMONIZED = false;
constexpr size_t EPOLL_EVENTS_SLOT = 512;
constexpr size_t CONNECTIONS_SLOT = 512;

// 时间粒度
constexpr mSec TIME_RESOLUTION = 100;
constexpr mSec ACCEPT_DELAY = 500;
constexpr mSec CLIENT_HEADER_TIMEOUT = 60000;
constexpr mSec TIMER_LAZY_DELAY = 300;


enum {
    HTTP_LINGERING_OFF      = 0,
    HTTP_LINGERING_ON       = 1,
    HTTP_LINGERING_ALWAYS   = 2
};

constexpr uInt HTTP_LINGERING = HTTP_LINGERING_ON;
constexpr mSec HTTP_LINGERING_TIME = 30000;
constexpr mSec HTTP_LINGERING_TIMEOUT = 5000;
constexpr size_t HTTP_LINGERING_BUFFER_SIZE = 4096;
constexpr mSec KEEPALIVE_TIME   = 3600000; // ngx_http_update_location_config
constexpr mSec KEEPALIVE_TIMEOUT = 75000;

// 若文件小于这个值，使用sendfile, 
constexpr off_t MIN_DIRECTIO_SIZE    = MAX_OFF_T_VALUE; // 4 * 1024 * 1024;
    // ngx_conf_merge_sec_value(conf->keepalive_header,
    //                           prev->keepalive_header, 0);
    // ngx_conf_merge_uint_value(conf->keepalive_requests,
    //                           prev->keepalive_requests, 1000);
    // ngx_conf_merge_msec_value(conf->resolver_timeout,
    //                           prev->resolver_timeout, 30000);

#endif