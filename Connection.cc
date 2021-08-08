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

    m_pool.destroy_pool();
    m_data = nullptr;
}

void Connection::create_http_connection()
{
    m_data = m_pool.malloc(sizeof(HttpConnection));
    
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

HttpRequest *Connection::create_http_request()
{
    HttpConnection *hc = static_cast<HttpConnection*>(m_data);
    Pool *pool = new Pool;

    if (pool == nullptr) {
        return nullptr;
    }

    HttpRequest *r = static_cast<HttpRequest*>(pool->calloc(1, sizeof(HttpRequest)));

    if (r == nullptr) {
        delete pool;
        return nullptr;
    }

    r->m_pool = pool; 

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
    return r;
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

void Connection::finalize_http_request(HttpRequest *r, Int rc)  // ngx_http_finalize_request
{
    if (rc == DONE) {
        finalize_http_connection(r);
        return;
    }

    if (rc == OK && r->m_filter_finalize) {
        m_error = true;
    }

    if (rc == DECLINED) {
        r->set_write_event_handler(&HttpRequest::run_phases_handler);
        r->run_write_event_handler();
        return;
    }

    //  if (r != r->main && r->post_subrequest) {
    //     rc = r->post_subrequest->handler(r, r->post_subrequest->data, rc);
    // }

    if (rc == ERROR 
        || rc == REQUEST_TIME_OUT
        || rc == CLIENT_CLOSED_REQUEST
        || m_error) 
    {  
        // if (ngx_http_post_action(r) == NGX_OK) {
        //     return;
        // }

        terminate_http_request(r, rc);
        return;
    }

    if (rc >= SPECIAL_RESPONSE
        || rc == CREATED
        || rc == NO_CONTENT)
    {
        if (rc == CLOSE) {
            m_timedout = true;
            terminate_http_request(r, rc);
            return;
        }

        if (r == r->m_main_request) {
            if (m_read_event->m_timer_set) {
                g_epoller->del_timer(m_read_event);
            }

            if (m_write_event->m_timer_set) {
                g_epoller->del_timer(m_write_event);
            }

            m_read_event->set_event_handler(&Event::http_request_handler);
            m_write_event->set_event_handler(&Event::http_request_handler);
            assert(0);
            // ngx_http_finalize_request(r, ngx_http_special_response_handler(r, rc));
            return;
        }
    }

    if (r != r->m_main_request) {
        assert(0);
    }

    if (r->m_buffered || m_buffered /*|| r->postponed*/) {
        assert(0);
        // if (ngx_http_set_write_handler(r) != NGX_OK) {
        //     ngx_http_terminate_request(r, 0);
        // }
        return;
    }

    if (r != m_data) {
        log_error(LogLevel::alert, "(%s: %d) http finalize non-active request\n", __FILE__, __LINE__);
        return;
    }

    r->m_done = true;
    
    r->set_read_event_handler(&HttpRequest::block_reading_handler);
    r->set_write_event_handler(&HttpRequest::empty_handler);

    if (r->m_post_action == false) {
        r->m_request_complete = true;
    }

    // if (ngx_http_post_action(r) == NGX_OK) {
    //     return;
    // }

    if (m_read_event->m_timer_set) {
        g_epoller->del_timer(m_read_event);
    }

    if (m_write_event->m_timer_set) {
        m_write_event->m_delayed = false;
        g_epoller->del_timer(m_write_event);
    }

    finalize_http_connection(r);
}

void Connection::free_http_request(HttpRequest *r, Int rc)
{
    if (r->m_pool == nullptr) {
        log_error(LogLevel::alert, "(%s: %d)  http request already closed\n", __FILE__, __LINE__);
        return;
    }

    if (rc > 0 && (r->m_header_out.m_status == 0 || m_sent == 0)) {
        r->m_header_out.m_status = rc;
    }

    if (m_timedout) {
        if (RESET_TIMEDOUT_CONNECTION) {
            linger linger;

            linger.l_onoff = 1;
            linger.l_linger = 0;

            /* 执行close() 时，若传输层发送缓冲区有数据，则丢弃全部数据 */
            if (setsockopt(m_fd, SOL_SOCKET, SO_LINGER, 
                            &linger, sizeof(struct linger)) == -1)
            {
                log_error(LogLevel::alert, "(%s, %d) setsockopt(SO_LINGER) failed",
                            __FILE__, __LINE__);
            }
        }
    }

    r->m_request_line.m_len = 0;
    m_destroyed = true;

    Pool *pool = r->m_pool;
    r->m_pool = nullptr;

    pool->destroy_pool();
}

void Connection::finalize_http_connection(HttpRequest *r)
{
    if (r->m_main_request->m_count != 1) {
        // if (r->discard_body) {
        //     r->read_event_handler = ngx_http_discarded_request_body_handler;
        //     ngx_add_timer(r->connection->read, clcf->lingering_timeout);

        //     if (r->lingering_time == 0) {
        //         r->lingering_time = ngx_time()
        //                               + (time_t) (clcf->lingering_time / 1000);
        //     }
        // }
        assert(0);
        close_http_request(r, 0);
        return;
    }

    r = r->m_main_request;
    
    // TODO 假设 r->m_main_request->m_connection == r->m_connection
    // 假设不成立的话，所有Connection成员变量都可能不对

    if (r->m_connection->m_read_event->m_eof) { 
        close_http_request(r, 0);
        return;
    }

    if (r->m_reading_body) {
        r->m_keepalive = false;
        r->m_lingering_close = true;
    }

    if (g_process->m_sig_terminate == 0
        && !g_cycle->m_exiting
        && r->m_keepalive 
        && KEEPALIVE_TIMEOUT > 0) 
    {
        set_http_keepalive(r);
        return;
    }

    if (HTTP_LINGERING_ALWAYS 
        || (HTTP_LINGERING_ON
            && (r->m_lingering_close
                || r->m_header_in_buffer->m_pos < r->m_header_in_buffer->m_last
                || r->m_connection->m_read_event->m_ready))) 
    {
        set_http_lingering_close();
        return;
    }

    close_http_request(r, 0);
}

