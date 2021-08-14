#ifndef _TIMER_H_INCLUDED_
#define _TIMER_H_INCLUDED_
#include "Config.h"
#include "StringSlice.h"
class Timer {
public:
    Timer();
    ~Timer();
    void update_time();
static int get_http_time(time_t sec, u_char *buf);
static int get_http_time(struct tm *tm, u_char *buf);
    
    StringSlice cached_http_time() const;
    time_t cached_time_sec() const;
    mSec   cached_time_msec() const;
private:
    static const char  *s_week[];
    static const char  *s_month[];
    size_t              m_slot;

    struct {
        time_t  m_sec;
        uInt    m_msec; 
        Int     m_gmtoff;
    }                   m_cached_time[TIME_SLOTS];

    u_char              m_cached_http_time[TIME_SLOTS][sizeof("Mon, 28 Sep 1970 06:00:00 GMT")];

};
#endif