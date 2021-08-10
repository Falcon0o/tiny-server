#include "HttpHeadersOut.h"


std::unordered_map<unsigned, StringSlice> HttpHeadersOut::s_content_types = 
{
    {hash_string("png"), STRING_SLICE("image/png")}
};

