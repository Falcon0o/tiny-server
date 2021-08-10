#ifndef _STRING_SLICE_IN_H_INCLUDED_
#define _STRING_SLICE_IN_H_INCLUDED_

#include "Config.h"

class StringSlice {
public:
    StringSlice();
    StringSlice(const StringSlice &);
    StringSlice &operator=(const StringSlice &);

    StringSlice(u_char *data, size_t len);
    StringSlice(StringSlice &&);

    size_t hash()const;
    bool operator==(const char*) const;
    StringSlice &operator=(const char *c);
    u_char          *m_data;
    size_t           m_len;
};

#define STRING_SLICE(c) StringSlice((u_char*)c, sizeof(c) - 1)

void str_printf(const StringSlice &s);

void str_lowcase(u_char *dst, u_char *src, size_t len);
StringSlice GetHttpStatusLines(uInt http_status);


#endif