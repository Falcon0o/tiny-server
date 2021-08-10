#include "Event.h"

#include "Buffer.h"
#include "Connection.h"
#include "Connections.h"

#include "Cycle.h"
#include "Epoller.h"
#include "HttpConnection.h"
#include "HttpRequest.h"
#include "Listening.h"
#include "Pool.h"
#include "Timer.h"

void Event::accept_handler()
{
    if (m_timeout) {
        if (g_cycle->enable_all_accept_events() != OK)
        {
            return;
        }
        m_timeout = false;
    }

    Connection *const ls_conn = m_connection;
    Listening *ls = ls_conn->m_listening;

    m_ready = false;

    do {
        sockaddr sa;
        socklen_t socklen = sizeof(sa);
    
        int s = accept4(ls->m_fd, &sa, &socklen, SOCK_NONBLOCK);

        if (s == -1) {

            int err = errno;
            switch (err) 
            {
            case EAGAIN:
                // log_error0(LogLevel::debug, "accept() not ready.\n");
                return;

            case ECONNABORTED:
                log_error(LogLevel::debug, "(%s: %d) ECONNABORTED \n", 
                                            __FILE__, __LINE__);
                if (m_available_n) {
                    continue;
                }
                break;
            
            case EMFILE:
            case ENFILE:
                if (err == EMFILE) {
                    log_error(LogLevel::debug, "(%s: %d) ===> Too many open files.\n", 
                        __FILE__, __LINE__);

                } else {
                    log_error(LogLevel::debug, "(%s: %d) ===> File table overflow.\n", 
                        __FILE__, __LINE__);
                }

                if (g_cycle->disable_all_accept_events() != OK) {
                    return;
                }

                g_epoller->add_timer(this, ACCEPT_DELAY);
                break;
            }
            return;
        }
        
        /* s != -1 */
        log_error(LogLevel::info, "worker %ld pid %d: 新的连接 (%s: %d)\n",  
            g_cycle->worker_id(),  g_process->m_pid, __FILE__, __LINE__);
        
        if (sa.sa_family == AF_INET) {
            sockaddr_in *sin = reinterpret_cast<sockaddr_in*>(&sa);
            u_char *uc = reinterpret_cast<u_char*>(&sin->sin_addr.s_addr);
            log_error(LogLevel::info, " %ud.%ud.%ud.%ud : %d (%s: %d)\n", uc[0], uc[1], uc[2], uc[3], ntohs(sin->sin_port), __FILE__, __LINE__); 
        }

        Connection *c = g_connections->get_connection(s);
        if (c == nullptr) {
            log_error(LogLevel::debug, "%s: %d\n", __FILE__, __LINE__);
            if (close(s) == -1) {
                log_error(LogLevel::debug, "%s: %d\n", __FILE__, __LINE__);
            }
            return;
        }

        c->m_type = SOCK_STREAM;
        
        c->m_sockaddr = static_cast<sockaddr*>(malloc(sizeof(sockaddr)));
        
        if (c->m_sockaddr == nullptr) {
            c->close_accepted_connection();
            return;
        }

        memcpy(c->m_sockaddr, &sa, sizeof(sockaddr));

        c->m_listening = ls;

        Event *rev = c->m_read_event;
        Event *wev = c->m_write_event;

        wev->m_ready = true;

        if (rev->m_deferred_accept) {
            rev->m_ready = true;
            rev->m_available_n = 1;
        }

        c->m_start_time = g_curr_msec;

        if (g_epoller->add_connection(c) == ERROR) {
            c->close_accepted_connection();
            return;
        }
        
        c->create_http_connection();
    } while(m_available_n);
}

void Event::empty_handler()
{
    return;
}


void Event::wait_http_request_handler()
{
    Connection *const c = m_connection;
    if (m_timeout) {
        log_error(LogLevel::debug, "(%s: %d) client timed out\n", __FILE__, __LINE__);
        c->close_http_connection();
        return;
    }
    
    if (c->m_close) {
        log_error(LogLevel::debug, "%s: %d\n", __FILE__, __LINE__);
        c->close_http_connection();
        return;
    }

    HttpConnection *hc = c->m_data.hc;
    
    Buffer *buf = c->m_small_buffer;
    if (buf == nullptr) {
        buf = Buffer::create_temp_buffer(&c->m_pool, CLIENT_HEADER_BUFFER_SIZE);

        if (buf == nullptr) {
            c->close_http_connection();
            return;
        }

        c->m_small_buffer = buf;
    
    } else if (buf->m_start == nullptr) {

        buf->m_start = static_cast<u_char*>(c->m_pool.malloc(CLIENT_HEADER_BUFFER_SIZE));
        if(buf->m_start == nullptr) {
            c->close_http_connection();
            return;
        }

        buf->m_pos = buf->m_start;
        buf->m_last = buf->m_start;
        buf->m_end = buf->m_start + CLIENT_HEADER_BUFFER_SIZE;
    }

    ssize_t n = c->recv_to_buffer(buf);

    if (n == AGAIN) {
        
        if (!m_timer_set) {
            g_epoller->add_timer(this, CLIENT_HEADER_TIMEOUT);
            g_connections->reusable_connection(c, true);
        }

        if (!m_active && !m_ready) {
            if (g_epoller->add_read_event(this, EPOLLET) == ERROR) {
                c->close_http_connection();
                return;
            }
        }
        /*
         * We are trying to not hold c->buffer's memory for an idle connection.
         */

        c->m_pool.free(buf->m_start);
        buf->m_start = nullptr;

        return;
    }

    if (n == ERROR) {
        log_error(LogLevel::debug, "%s: %d\n", __FILE__, __LINE__);
        c->close_http_connection();
        return;
    }

    if (n == 0) {
        log_error(LogLevel::debug, "    客户端关闭了连接 (%s: %d)\n", __FILE__, __LINE__);
        c->close_http_connection();
        return;
    }

    buf->m_last += n;

    g_connections->reusable_connection(c, false);

    HttpRequest *r = c->create_http_request();
    if (r == nullptr) { 
        log_error(LogLevel::debug, "%s: %d\n", __FILE__, __LINE__);
        c->close_http_connection();
        return;
    }
    
    c->m_data.r = r;

    set_handler(&Event::process_http_request_line_handler);
    run_handler();
}

void Event::http_request_handler() {
    Connection *const c = m_connection;
    
    if (c->m_close) {
        c->m_data.r->m_main_request->m_count++;
        c->terminate_http_request(0);
        c->run_posted_http_requests();
        return;
    }

    if (m_delayed && m_timeout) {
        m_delayed = false;
        m_timeout = false;
    }

    if (m_write) {
        c->m_data.r->run_write_handler();
    
    } else {
        c->m_data.r->run_read_handler();
    }

    c->run_posted_http_requests();
}

void Event::http_keepalive_handler()
{
    Connection *const c = m_connection;

    if (m_timeout || c->m_close) {
        c->close_http_connection();
        return;
    }

    Buffer *b = c->m_small_buffer;

    if (b->m_pos == nullptr) {
        /*
         * The c->buffer's memory was freed by ngx_http_set_keepalive().
         * However, the c->buffer->start and c->buffer->end were not changed
         * to keep the buffer size.
         */
        b->m_pos = (u_char*)c->m_pool.malloc(b->m_end - b->m_start);
        if (b->m_pos == nullptr) {
            c->close_http_connection();
            return;
        }

        b->m_end    = b->m_pos + (b->m_end - b->m_start);
        b->m_start  = b->m_pos;
        b->m_last   = b->m_pos;
    }

    /*
     * MSIE closes a keepalive connection with RST flag
     * so we ignore ECONNRESET here.
     */
    // c->log_error = NGX_ERROR_IGNORE_ECONNRESET;
    errno = 0;

    ssize_t n = c->recv_to_buffer(b);

    if (n == AGAIN) {
        if (!m_active && !m_ready) {
            if (g_epoller->add_read_event(this, EPOLLET) != OK) {
                c->close_http_connection();
                return;
            }
        }

        c->m_pool.free(b->m_start);
        b->m_pos = nullptr;
        return;
    }

    if (n == ERROR) {
        c->close_http_connection();
        return;
    }

    if (n == 0) {
        log_error(LogLevel::debug, "(%s: %d) client closed keepalive connection\n", __FILE__, __LINE__);
        c->close_http_connection();
        return;
    }

    b->m_last += n;

    c->m_idle = false;
    g_connections->reusable_connection(c, false);
    c->m_data.r = c->create_http_request();

    if (c->m_data.r == nullptr) {
        c->close_http_connection();
        return;
    }

    c->m_sent = 0;
    c->m_destroyed = false;
    
    g_epoller->del_timer(c->m_read_event);

    c->m_read_event->set_handler(&Event::process_http_request_line_handler);
    c->m_read_event->run_handler();
}


void Event::http_lingering_close_handler()
{
    Connection *const c = m_connection;

    if (m_timeout || c->m_close) {
        c->close_http_request(0);
        return;
    }

    time_t timer = c->m_data.r->m_lingering_time - g_timer->cached_time_sec();
    if (timer < 0) {
        c->close_http_request(0);
        return;
    }
    u_char  buffer[HTTP_LINGERING_BUFFER_SIZE];
    do {
        ssize_t n = c->recv_to_buffer(buffer, buffer + HTTP_LINGERING_BUFFER_SIZE);

        if (n == AGAIN) {
            break;
        }

        if (n == ERROR || n == 0) {
            c->close_http_request(0);
            return;
        }

    } while(m_ready);

    mSec msec = timer * 1000;
    msec = msec > HTTP_LINGERING_TIMEOUT ? HTTP_LINGERING_TIMEOUT : msec;
    g_epoller->add_timer(this, msec);
}



void Event::process_http_request_line_handler()
{
    Connection *const c = m_connection;

    HttpRequest *r = c->m_data.r;

    if (m_timeout) {
        log_error(LogLevel::debug, "(%s: %d) client timed out\n", __FILE__, __LINE__);
        c->m_timedout = true;
        c->close_http_request(REQUEST_TIME_OUT);
        return;
    }
    
    for (Int rc = AGAIN; ; ) {

        if (rc == AGAIN) {
            ssize_t n = r->read_request_header();
            if (n == AGAIN || n == ERROR) {
                break;
            }
        }

        rc = r->parse_request_line();

        if (rc == OK) {
            
            r->m_request_line.m_len     = r->m_request_end - r->m_request_start;
            r->m_request_line.m_data    = r->m_request_start;
            r->m_request_length         = r->m_header_in_buffer->m_pos - r->m_request_start;

            r->m_method_name.m_len = r->m_method_end - r->m_request_start/* + 1 */;
            r->m_method_name.m_data = r->m_request_line.m_data;

            /* HTTP 09 没有以下部分 */
            if (r->m_http_protocol_start) {
                r->m_http_protocol.m_data = r->m_http_protocol_start;
                r->m_http_protocol.m_len = r->m_request_end - r->m_http_protocol_start;
            }

            r->m_uri.m_data = r->m_uri_start;
            r->m_uri.m_len  = r->m_uri_end - r->m_uri_start;

            if (r->m_uri_ext_start) {
                r->m_uri_ext.m_data = r->m_uri_ext_start;

                if (r->m_uri_ext_end) {
                    r->m_uri_ext.m_len = r->m_uri_ext_end - r->m_uri_ext_start;
                
                } else {
                    r->m_uri_ext.m_len = r->m_uri_end - r->m_uri_ext_start;
                }
                
            }
            if (r->process_request_uri() != OK) {
                break;
            }

            set_handler(&Event::process_http_request_headers_handler);
            run_handler();
            break;
        }

        if (rc != AGAIN) {
            if (rc == PARSE_INVALID_VERSION) {
                c->finalize_http_request(r, VERSION_NOT_SUPPORTED);

            } else {
                c->finalize_http_request(r, BAD_REQUEST);
            }

            break;
        }

        /* NGX_AGAIN: a request line parsing is still incomplete */
        if (r->need_alloc_large_header_buffer()) {
            Int rv = r->alloc_large_header_buffer(true);

            if (rv == ERROR) {
                c->finalize_http_request(r, INTERNAL_SERVER_ERROR);
                break;
            }

            if (rv == DECLINED) {
                r->m_request_line.m_len = r->m_header_in_buffer->m_end - r->m_request_start;
                r->m_request_line.m_data = r->m_request_start;
                c->finalize_http_request(r, REQUEST_URI_TOO_LARGE);
                break;
            }
        }   
    }

    c->run_posted_http_requests();
}






    
void Event::process_http_request_headers_handler() 
{
    Connection *const c = m_connection;
    HttpRequest *r = c->m_data.r;
    if (m_timeout) {
        log_error(LogLevel::debug, "%s: %d\n", __FILE__, __LINE__);
        c->m_timedout = true;
        c->close_http_request(REQUEST_TIME_OUT);
        return;
    }

    Int rc = AGAIN;

    for ( ;; ) {
        
        if (rc == AGAIN) {
            if (r->need_alloc_large_header_buffer()) {
                Int rv = r->alloc_large_header_buffer(false);
                
                if (rv == ERROR) {
                    c->close_http_request(INTERNAL_SERVER_ERROR);
                    break;
                }

                if (rv == DECLINED) {
                    if (r->m_header_name_start == nullptr) {
                        log_error(LogLevel::info, "%s: %d  client sent too large request\n", 
                                            __FILE__, __LINE__);
                        c->finalize_http_request(r, REQUEST_HEADER_TOO_LARGE);
                        break;
                    }
                    log_error(LogLevel::info, "%s: %d client sent too long header line\n", 
                                            __FILE__, __LINE__);
                    c->finalize_http_request(r, REQUEST_HEADER_TOO_LARGE);
                    break;
                }

            }

            ssize_t n = r->read_request_header();
            
            if (n == AGAIN || n == ERROR) {
                break;
            }
        }
        rc = r->parse_header_line();

        if (rc == OK) {
            r->m_request_length += r->m_header_in_buffer->m_pos - r->m_header_name_start;
            if (r->m_invalid_header) {
                continue;
            }

            StringSlice key(r->m_header_name_start, r->m_header_name_end - r->m_header_name_start);
            key.m_data[key.m_len] = '\0';

            StringSlice value(r->m_header_start, r->m_header_end - r->m_header_start);
            value.m_data[value.m_len] = '\0';

            void *lowcase_key = r->m_pool->malloc(key.m_len);
            if (lowcase_key == nullptr) {
                c->close_http_request(INTERNAL_SERVER_ERROR);
                break;
            }

            if (key.m_len == r->m_lowcase_index) {
                memcpy(lowcase_key, r->m_lowcase_header, key.m_len);
            } else {
                str_lowcase((u_char*)lowcase_key, key.m_data, key.m_len);
            }

            HttpHeadersIn::iterator iter = r->m_header_in.push_front_header(
                    key, StringSlice((u_char*)lowcase_key, key.m_len), value, r->m_header_hash);
            
            if (r->m_header_in.handle_header(iter, r) != OK) {
                break;
            }
            continue;
        }

        if (rc == PARSE_HEADER_DONE) {
            r->m_request_length += r->m_header_in_buffer->m_pos - r->m_header_name_start;
            
            rc = r->process_request_header();
            if (rc != OK) {
                break;
            }

            r->process_request();
            break;
        }

        if (rc == AGAIN) {
            continue;
        }

        log_error(LogLevel::info, "%s: %d client sent invalid header line\n", 
                                            __FILE__, __LINE__);
        c->finalize_http_request(r, BAD_REQUEST);
        break;
    }
    c->run_posted_http_requests();
}

// void http_block_reading_handler(Event *ev)
// {
//     Connection *conn = ev->get_data<Connection*>();
//     HttpRequest *req = conn->get_data<HttpRequest*>();

//     if (conn->close()) {
//         // TODO: 
//         req->terminate_with_status_code(HttpStatus::UNSET);
//         // ngx_http_run_posted_requests(c);
//         return;
//     }

//     if (ev->delayed() && ev->timeout()) {
//         ev->set_delayed(false);
//         ev->set_timeout(false);
//     }

//     // 啥也不做
// }
