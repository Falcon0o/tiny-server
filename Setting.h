#ifndef _SETTING_H_INCLUDED_
#define _SETTING_H_INCLUDED_

#include "Types.h"

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
constexpr size_t IOVS_PREALLOCATE = 64;
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
constexpr mSec SEND_TIMEOUT = 60000;


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


constexpr time_t    OPEN_FILE_CACHE_VALID = 60;
constexpr time_t    OPEN_FILE_CACHE_INACTIVE = 60;

constexpr uInt      OPEN_FILE_CACHE_MAX = 3;
constexpr uInt      OPEN_FILE_CACHE_MIN_USES = 1;

constexpr rlim_t     WORKER_RLIMIT_NOFILE    = 0;
constexpr rlim_t     WORKER_RLIMIT_CORE      = 10000;


#endif