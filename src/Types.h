#ifndef _TYPES_H_INCLUDED_
#define _TYPES_H_INCLUDED_

#include "LinuxConfig.h"

#define MAX_TIME_T_VALUE    9223372036854775807LL
#define MAX_OFF_T_LEN       (sizeof("-9223372036854775808") - 1)
#define MAX_OFF_T_VALUE     9223372036854775807LL
#define MAX_TIME_T_LEN      (sizeof("-9223372036854775808") - 1)
#define MAX_SENDFILE_SIZE   2147483647L

using Int       = intptr_t;
using uInt      = uintptr_t;
using mSec      = uInt;


enum {
    HTTP_LINGERING_OFF      = 0,
    HTTP_LINGERING_ON       = 1,
    HTTP_LINGERING_ALWAYS   = 2
};

#endif