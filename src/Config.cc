#include "Config.h"

#include <cassert>


const void *ERROR_ADDR_token = "RC_ERROR";
const void *DECLINED_ADDR_token = "RC_DECLINED";


static_assert(LARGE_CLIENT_HEADER_BUFFER_SIZE > CLIENT_HEADER_BUFFER_SIZE, 
"LARGE_CLIENT_HEADER_BUFFER_SIZE 应大于 CLIENT_HEADER_BUFFER_SIZE");

size_t hash(size_t key, u_char c)
{
    return key * 31 + c;
}

size_t hash_string(const char *c) {
    const char * pos = c;

    unsigned ans = 0;
    while (*pos) {
        ans = hash(ans, *pos);
        ++pos;
    }
    return ans;
}

void debug_point() {
    
    int err = errno;
    fprintf(stdout, "errno %d : %s\n", err, strerror(err));
    fflush(stdout);
    abort();
}


