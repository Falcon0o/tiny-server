#include "Connection.h"

#include "Buffer.h"
#include "BufferChain.h"
#include "Cycle.h"
#include "Epoller.h"
#include "HttpConnection.h"
#include "HttpRequest.h"
#include "Timer.h"


void Connection::close_accepted_connection()
{
    g_cycle->free_connection(this);

    int fd = m_fd;
    m_fd = -1;
    
    if (close(fd) == -1) {
        log_error(LogLevel::alert, "%s: %d\n", __FILE__, __LINE__);
    }
}  

void Connection::close_connection()
{
    if (m_fd == -1) {
        log_error(LogLevel::alert, "(%s: %d) connection already closed\n", __FILE__, __LINE__);
        return;
    }
    
    if (m_read_event->m_timer_set) {
        g_epoller->del_timer(m_read_event);
    }

    if (m_write_event->m_timer_set) {
        g_epoller->del_timer(m_write_event);
    }

    if (!m_shared) {
        g_epoller->del_connection(this, true);
    }

    // TODO
    //  if (c->read->posted) {
    //     ngx_delete_posted_event(c->read);
    // }

    // if (c->write->posted) {
    //     ngx_delete_posted_event(c->write);
    // }

    m_read_event->m_closed = true;
    m_write_event->m_closed = true;

    g_cycle->reusable_connection(this, false);
    g_cycle->free_connection(this);

    int fd = m_fd;
    m_fd = -1;

    if (m_shared) {
        return;
    }

    if (close(fd) == -1) {
        int err = errno;
        if (err == ENOTCONN || err == ECONNRESET)
        {
            // TODO
        }
        log_error(LogLevel::alert, "(%s: %d) errno: %d\n", __FILE__, __LINE__, err);
    }
}


void Connection::close_http_connection() {
    m_destroyed = true;
    close_connection();

    for (auto iter = m_pool.begin(); iter != m_pool.end(); ++iter) {
        void *addr = *iter;
        free(addr);
    }
    
    m_pool.clear();
    m_data = nullptr;
}

void *Connection::pool_malloc(size_t s) {
    void *ans = malloc(s);
    if (ans) {
        m_pool.insert(ans);
    }
    return ans;
}

void *Connection::pool_calloc(size_t n, size_t size) {
    void *ans = calloc(n, size);
    if (ans) {
        m_pool.insert(ans);
    }
    return ans;
}


void Connection::pool_free(void *addr) {   
    free(addr);
    m_pool.erase(addr);
}

void Connection::create_http_connection()
{
    m_data = pool_malloc(sizeof(HttpConnection));

    if (m_data == nullptr) {
        close_http_connection();
        return;
    }
    new (m_data)HttpConnection;
    
    m_read_event->set_event_handler(&Event::wait_http_request_handler);
    m_write_event->set_event_handler(&Event::empty_handler);

    if (m_read_event->m_ready) {
        m_read_event->run_event_handler();
        return;
    }
    
    g_epoller->add_timer(m_read_event, CLIENT_HEADER_TIMEOUT);
    g_cycle->reusable_connection(this, true);
}

ssize_t Connection::recv_to_buffer(Buffer *b)
{
    Event *rev = m_read_event;

    if (rev->m_available_n == 0 && !rev->m_pending_eof) {
        /* 如果接收缓冲区没有数据且未触发EPOLLRDHUP，当前事件不可读 */
        rev->m_ready = false;
        return AGAIN;
    }

    ssize_t n;
    int err;
    do {
        n = ::recv(m_fd, b->m_last, b->m_end - b->m_last, 0);

        if (n == 0) {
            rev->m_ready = false;
            rev->m_eof = true;
            return 0;
        }

        if (n > 0) {
            if (rev->m_available_n >= 0) {
                rev->m_available_n -= n;
                
                if (rev->m_available_n < 0) {
                    rev->m_available_n = 0;
                    rev->m_ready = false;
                }

            } else if (n == static_cast<ssize_t>(b->m_end - b->m_last)) {
                
                int nread;

                if (ioctl(m_fd, FIONREAD, &nread) == -1) {
                    n = ERROR;
                    log_error(LogLevel::alert, "(%s: %d) ioctl(FIONREAD) 失败\n", __FILE__, __LINE__);
                    break;
                }
                rev->m_available_n = nread;
            }

            if (n < static_cast<ssize_t>(b->m_end - b->m_last)) {
                if (!rev->m_pending_eof) {
                    rev->m_ready = false;
                }
                rev->m_available_n = 0;
            }

            return n;
        }

        /* n < 0 */
        err = errno;
        if (err == EAGAIN || err == EINTR) {
            n = AGAIN;
        }
        else {
            n = ERROR;
            break;
        }

    } while(err == EINTR);

    rev->m_ready = false;

    if (n == ERROR)
    {
        rev->m_err = true;
    }

    return n;
}

void Connection::create_http_request()
{
    HttpRequest *r = static_cast<HttpRequest*>(pool_calloc(1, sizeof(HttpRequest)));

    if (r == nullptr) {
        return;
    }

    HttpConnection *hc = static_cast<HttpConnection*>(m_data);
    m_data = nullptr;

    r->m_http_connection = hc;
    r->m_connection = this;
    r->set_read_event_handler(&HttpRequest::block_reading_handler);

    r->m_header_in_buffer = hc->m_busy_buffers ? hc->m_busy_buffers->m_buffer : m_small_buffer;
    r->m_main_request = r;
    r->m_count = 1;

    r->m_start_sec = g_timer->cached_time_sec();
    r->m_start_msec = g_timer->cached_time_msec();

    r->m_method = HttpRequest::Method::UNKNOWN;
    r->m_http_version = 1000;

    r->m_header_in.m_content_length_n = -1;
    r->m_header_in.m_keep_alive_n = -1;

    r->m_header_out.m_content_length_n = -1;
    r->m_header_out.m_last_modified_time = -1;

    // r->uri_changes = NGX_HTTP_MAX_URI_CHANGES + 1;
    // r->subrequests = NGX_HTTP_MAX_SUBREQUESTS + 1;

    // ngx_http_alloc_request

    ++m_request_cnt;

    m_data = r;
}

// BufferChain *Connection::sendfile_chain(BufferChain *in, off_t limit)
// { // ngx_linux_sendfile_chain
//     int pagesize = Cycle::singleton()->getpagesize();

//     if (!m_wev->ready()) {
//         // 不可写
//         return in;
//     }


//     if (limit == 0 || limit > static_cast<off_t>(MAX_SENDFILE_SIZE - pagesize))
//     {
//         limit = MAX_SENDFILE_SIZE - pagesize;
//     }


//     iovec headers[IOVS_PREALLOCATE];
//     IOVector io_vector(headers, IOVS_PREALLOCATE);

//     off_t aim_send = 0, prev_aim_send;
//     size_t real_sent = 0;

//     for ( ;; ) {

//         prev_aim_send = aim_send;
//         BufferChain *chain_last = io_vector.fill_with_output_chain(*in, limit - aim_send);

//         if (chain_last == ERROR_ADDR(BufferChain)) {
//             return chain_last;
//         }

//         aim_send += io_vector.total_length();

//         if (tcp_nopush() == UNSET 
//             && io_vector.size() != 0                            /* 内存中含有数据 */
//             && chain_last && chain_last->buffer()->in_file())   /* 最后一个未处理的缓冲区在文件中 */
//         {
//             /* 应该关闭nodelay，即启用nagle算法， 尝试将 m_tcp_nodelay 设置为UNSET，
//              * 将 tcp_nopush 设置为 SET */
//             if (tcp_nodelay() == ON) { 
//                 int on = 0;
//                 if (setsockopt(m_fd, IPPROTO_TCP, TCP_NODELAY,
//                         &on, sizeof(int)) == -1) 
//                 {
//                     int err = errno;
//                     /*
//                      * there is a tiny chance to be interrupted, however,
//                      * we continue a processing with the TCP_NODELAY
//                      * and without the TCP_CORK
//                      */
//                     if (err != EINTR) {
//                         m_wev->set_err(true);
//                         return ERROR_ADDR(BufferChain);
//                     }

//                 } else {
//                     m_tcp_nodelay = UNSET;
//                 }
//             }
            
//             if (m_tcp_nodelay == UNSET) {
//                 int on = 1;
//                 if (setsockopt(m_fd, IPPROTO_TCP, TCP_CORK,
//                         &on, sizeof(int)) == -1) 
//                 {
//                     int err = errno;
//                     if (err != EINTR) {
//                         m_wev->set_err(true);
//                         return ERROR_ADDR(BufferChain);
//                     }

//                 } else {
//                     m_tcp_nopush = ON;
//                 } 
//             }
//         }

//         ssize_t n;
//         if (io_vector.size() == 0 && chain_last && chain_last->buffer()->in_file() && aim_send < limit) 
//         {
//             Buffer *buf = chain_last->buffer();
//             /* 没有内存中的内容，纯文件，*/
//             off_t file_size;
//             chain_last = chain_last->coalesce_file(limit - aim_send, file_size);

//             aim_send += file_size;

//             if (file_size == 0) {
//                 debug_point();
//                 return ERROR_ADDR(BufferChain);
//             }

//             n = sendfile_from_buffer(buf, file_size);

//             if (n == RC_ERROR) {
//                 return ERROR_ADDR(BufferChain);
//             }

//             // if (n == NGX_DONE) {
//             real_sent = (n == RC_AGAIN) ? 0 : n;
        
//         } else {
//             n = write_from_iovec(&io_vector);
//             if (n == RC_ERROR) {
//                 return ERROR_ADDR(BufferChain);
//             }
//             real_sent = (n == RC_AGAIN) ? 0 : n;
//         }

//         if (n == RC_AGAIN) {
//             m_wev->set_ready(false);
//             return in;
//         }


//         if (in) {
//             in = in->update_sent(real_sent);
//         }
        

//         if ((size_t)(aim_send - prev_aim_send) != real_sent) {
//             aim_send = prev_aim_send + real_sent;
//             continue;
//         }
//         if (aim_send >= limit || in == nullptr) {
//             return in;
//         }
//     }
// }

void Connection::close_http_request(HttpRequest *r, Int rc) // ngx_http_close_request
{
    if (r->m_count == 0) {
        // TODO
    }

    r->m_count--;

    if (r->m_count || r->m_blocked)  {
        return;
    }

    free_http_request(r, rc);
    close_connection();
}