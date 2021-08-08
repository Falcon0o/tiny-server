#ifndef _STRING_SLICE_IN_H_INCLUDED_
#define _STRING_SLICE_IN_H_INCLUDED_

#include "Config.h"

class StringSlice {
public:
    StringSlice();

    StringSlice(u_char *data, size_t len);
    StringSlice(StringSlice &&);

    u_char          *m_data;
    size_t           m_len;
};

#define STRING_SLICE(c) StringSlice((u_char*)c, sizeof(c) - 1)

StringSlice GetHttpStatusLines(uInt http_status);
#endif