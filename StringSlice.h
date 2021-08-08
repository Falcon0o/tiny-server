#ifndef _STRING_SLICE_IN_H_INCLUDED_
#define _STRING_SLICE_IN_H_INCLUDED_

#include "Config.h"

class StringSlice {
public:
    u_char          *m_data;
    size_t           m_len;
};
#endif