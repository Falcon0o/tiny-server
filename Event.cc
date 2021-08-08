#include "Event.h"

#include "Buffer.h"

#include "Cycle.h"
#include "Epoller.h"
#include "HttpConnection.h"
#include "HttpRequest.h"
#include "Listening.h"


void Event::accept_handler()
{
    if (m_timeout) {
        if (g_cycle->enable_all_accept_events() != OK)
        {
            return;
        }
        m_timeout = false;
    }

    Connection *ls_conn = static_cast<Connection*>(m_data);
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

        Connection *c = g_cycle->get_connection(s);
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
    Connection *conn = static_cast<Connection*>(m_data);
    if (m_timeout) {
        log_error(LogLevel::debug, "(%s: %d) client timed out\n", __FILE__, __LINE__);
        conn->close_http_connection();
        return;
    }
    
    if (conn->m_close) {
        log_error(LogLevel::debug, "%s: %d\n", __FILE__, __LINE__);
        conn->close_http_connection();
        return;
    }

    HttpConnection *hc = static_cast<HttpConnection*>(conn->m_data);
    
    Buffer *buf = conn->m_small_buffer;
    if (buf == nullptr) {
        buf = Buffer::create_temp_buffer(conn, CLIENT_HEADER_BUFFER_SIZE);

        if (buf == nullptr) {
            conn->close_http_connection();
            return;
        }

        conn->m_small_buffer = buf;
    
    } else if (buf->m_start == nullptr) {

        buf->m_start = static_cast<u_char*>(conn->pool_malloc(CLIENT_HEADER_BUFFER_SIZE));
        if(buf->m_start == nullptr) {
            conn->close_http_connection();
            return;
        }

        buf->m_pos = buf->m_start;
        buf->m_last = buf->m_start;
        buf->m_end = buf->m_start + CLIENT_HEADER_BUFFER_SIZE;
    }

    ssize_t n = conn->recv_to_buffer(buf);

    if (n == AGAIN) {
        
        if (!m_timer_set) {
            g_epoller->add_timer(this, CLIENT_HEADER_TIMEOUT);
            g_cycle->reusable_connection(conn, true);
        }

        if (!m_active && !m_ready) {
            if (g_epoller->add_read_event(this, EPOLLET) == ERROR) {
                conn->close_http_connection();
                return;
            }
        }
        /*
         * We are trying to not hold c->buffer's memory for an idle connection.
         */

        conn->pool_free(buf->m_start);
        buf->m_start = nullptr;

        return;
    }

    if (n == ERROR) {
        log_error(LogLevel::debug, "%s: %d\n", __FILE__, __LINE__);
        conn->close_http_connection();
        return;
    }

    if (n == 0) {
        log_error(LogLevel::debug, "    客户端关闭了连接 (%s: %d)\n", __FILE__, __LINE__);
        conn->close_http_connection();
        return;
    }

    buf->m_last += n;

    g_cycle->reusable_connection(conn, false);

    conn->create_http_request();
    if (conn->m_data == nullptr) { 
        log_error(LogLevel::debug, "%s: %d\n", __FILE__, __LINE__);
        conn->close_http_connection();
        return;
    }

    set_event_handler(&Event::process_http_request_line_handler);
    run_event_handler();
}

void Event::process_http_request_line_handler()
{
    Connection *conn = static_cast<Connection*>(m_data);

    HttpRequest *r = static_cast<HttpRequest*>(conn->m_data);

    if (m_timeout) {
        log_error(LogLevel::debug, "(%s: %d) client timed out\n", __FILE__, __LINE__);
        conn->m_timedout = true;
        conn->close_http_request(r, REQUEST_TIME_OUT);
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

            if (r->process_request_uri() != OK) {
                break;
            }

            set_event_handler(&Event::process_http_request_headers_handler);
            run_event_handler();
            break;
        }

        if (rc != AGAIN) {
            if (rc == PARSE_INVALID_VERSION) {
                conn->finalize_http_request(r, VERSION_NOT_SUPPORTED);

            } else {
                conn->finalize_http_request(r, BAD_REQUEST);
            }

            break;
        }

        /* NGX_AGAIN: a request line parsing is still incomplete */
        if (r->need_alloc_large_header_buffer()) {
            Int rv = r->alloc_large_header_buffer(true);

            if (rv == ERROR) {
                conn->finalize_http_request(r, INTERNAL_SERVER_ERROR);
                break;
            }

            if (rv == DECLINED) {
                r->m_request_line.m_len = r->m_header_in_buffer->m_end - r->m_request_start;
                r->m_request_line.m_data = r->m_request_start;
                conn->finalize_http_request(r, REQUEST_URI_TOO_LARGE);
                break;
            }
        }   
    }
}
