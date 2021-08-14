#include "Timer.h"


mSec g_curr_msec = 0;

static Timer timer;
Timer *g_timer = &timer;

const char *Timer::s_week[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
const char *Timer::s_month[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                            "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };


Timer::Timer()
:   m_slot(0)
{
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC_COARSE, &ts);
    g_curr_msec = ts.tv_sec * 1000 + ts.tv_nsec / 1000000; 

    timeval tv;
    gettimeofday(&tv, nullptr);

    m_cached_time[m_slot].m_sec = tv.tv_sec;
    m_cached_time[m_slot].m_msec = tv.tv_usec / 1000;

    tm *tm =  gmtime(&m_cached_time[m_slot].m_sec);
    get_http_time(tm, m_cached_http_time[m_slot]);

    (void) localtime_r(&m_cached_time[m_slot].m_sec, tm);
    m_cached_time[m_slot].m_gmtoff = tm->tm_gmtoff / 60;
}

Timer::~Timer() {}


void Timer::update_time()
{
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC_COARSE, &ts);
    g_curr_msec = ts.tv_sec * 1000 + ts.tv_nsec / 1000000; 

    timeval tv;
    gettimeofday(&tv, nullptr);

    time_t sec = tv.tv_sec;
    uInt msec = tv.tv_usec / 1000;
    
    
    if (m_cached_time[m_slot].m_sec == sec) {
        m_cached_time[m_slot].m_msec = msec;
        return;
    }

    m_slot = (m_slot == TIME_SLOTS - 1) ? 0 : m_slot + 1;

    m_cached_time[m_slot].m_sec = sec;
    m_cached_time[m_slot].m_msec = msec;

    tm *tm =  gmtime(&m_cached_time[m_slot].m_sec);
    get_http_time(tm, m_cached_http_time[m_slot]);

    (void) localtime_r(&m_cached_time[m_slot].m_sec, tm);
    m_cached_time[m_slot].m_gmtoff = tm->tm_gmtoff / 60;
}

int Timer::get_http_time(struct tm *tm, u_char *buf)
{   
    return sprintf(reinterpret_cast<char*>(buf), 
                "%s, %02d %s %4d %02d:%02d:%02d GMT",
                s_week[tm->tm_wday],
                tm->tm_mday,
                s_month[tm->tm_mon],
                tm->tm_year + 1900,
                tm->tm_hour,
                tm->tm_min,
                tm->tm_sec);
}

int Timer::get_http_time(time_t sec, u_char *buf)
{
    tm *tm =  gmtime(&sec);
    return get_http_time(tm, buf);
}

time_t Timer::cached_time_sec() const
{
    return m_cached_time[m_slot].m_sec;
}

mSec Timer::cached_time_msec() const
{
    return m_cached_time[m_slot].m_msec;
}

StringSlice Timer::cached_http_time() const
{
    size_t slot = m_slot;
    return StringSlice((u_char*)m_cached_http_time[slot], sizeof("Mon, 28 Sep 1970 06:00:00 GMT"));
}