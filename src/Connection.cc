#include "Connection.h"

#include "Buffer.h"
#include "BufferChain.h"
#include "ConnectionPool.h"
#include "Cycle.h"
#include "Epoller.h"
#include "HttpConnection.h"
#include "HttpRequest.h"
#include "IOVector.h"
#include "Timer.h"
#include "Pool.h"
Connection::Connection() noexcept
:   m_fd(-1),
    m_rev(true), 
    m_wev(false),
    m_pool(nullptr)
{

}

Connection::~Connection()
{
    delete m_pool;
    m_pool = nullptr;
}
void Connection::reinitialize(int fd) noexcept {

    bool instance = m_rev.m_instance;
    m_rev.reinitialize(this, !instance, false);
    m_wev.reinitialize(this, !instance, true);

    m_data.v        = nullptr;
    m_fd            = fd;
    m_listening     = nullptr;
    m_type          = 0;
    m_buffered      = 0;

    m_close         = false;
    m_reusable      = false;
    m_shared        = false;
    m_destroyed     = false;
    m_timedout      = false;
    m_error         = false;
    m_tcp_nopush    = TCP_NOPUSH_UNSET;

    m_tcp_nodelay   = TCP_NODELAY_UNSET;
    m_idle          = false;
    m_sockaddr      = nullptr;
    m_start_time    = 0;
    m_small_buffer  = nullptr;
    m_sent          = 0;
    m_request_cnt   = 0;
    m_pool          = new Pool;
}


HttpConnection *Connection::create_http_connection() {

    HttpConnection *hc = (HttpConnection*)m_pool->malloc(sizeof(HttpConnection));
    
    if (hc == nullptr) {
        debug_point();
        return nullptr;
    }

    new (hc)HttpConnection;

    m_rev.set_handler(&Event::wait_http_request);
    m_wev.set_handler(&Event::empty_handler);
    
    return hc;
}

void Connection::finalize_http_connection()
{
    if (m_rev.m_eof) { 
        close_http_request(0);
        return;
    }

    if (m_data.r->m_reading_body) {
        m_data.r->m_keepalive = false;
        m_data.r->m_lingering_close = true;
    }

    if (g_process->m_sig_terminate == 0
        && !g_cycle->m_exiting
        && m_data.r->m_keepalive 
        && KEEPALIVE_TIMEOUT > 0) 
    {
        set_http_keepalive();
        return;
    }

    if (HTTP_LINGERING == HTTP_LINGERING_ALWAYS
        || (HTTP_LINGERING == HTTP_LINGERING_ON
            && (m_data.r->m_lingering_close
                || m_data.r->m_header_in_buffer->m_pos < m_data.r->m_header_in_buffer->m_last
                || m_rev.m_ready))) 
    {
        set_http_lingering_close();
        return;
    }

    close_http_request(0);
}

void Connection::close_http_connection() 
{
    m_destroyed = true;

    close_connection();

    m_pool->destroy_pool();
    m_data.v = nullptr;
}


void Connection::close_accepted_connection(){

    g_connection_pool->free_connection(this);

    int fd = m_fd;
    m_fd = -1;
    
    if (::close(fd) == -1) {
        int err = errno;
        debug_point();
    }
}  




void Connection::close_connection()
{
    if (m_fd == -1) {
        return;
    }
    
    if (m_rev.m_timer_set) {
        g_epoller->del_timer(&m_rev);
    }

    if (m_wev.m_timer_set) {
        g_epoller->del_timer(&m_wev);
    }
    
    if (!m_shared) {
        g_epoller->del_connection(this, true);
    }

    if (m_rev.m_posted) {
        g_epoller->del_posted_event(&m_rev);
    }

    if (m_wev.m_posted) {
        g_epoller->del_posted_event(&m_wev);
    }

    m_rev.m_closed = true;
    m_wev.m_closed = true;

    g_connection_pool->free_connection(this);

    int fd = m_fd;
    m_fd = -1;

    if (m_shared) {
        return;
    }

    if (::close(fd) == -1) {
        debug_point();
    }
}

ssize_t Connection::recv_to_buffer(u_char *last, u_char *end)
{
    if (m_rev.m_available_n == 0 && !m_rev.m_pending_eof) {
        /* 如果接收缓冲区没有数据且未触发EPOLLRDHUP，当前事件不可读 */
        m_rev.m_ready = false;
        return AGAIN;
    }
    
    ssize_t n;
    int err;
    do {
        n = ::recv(m_fd, last, end - last, 0);

        if (n == 0) {
            m_rev.m_ready = false;
            m_rev.m_eof = true;
            return 0;
        }

        if (n > 0) {

            if (m_rev.m_available_n >= 0) {
                m_rev.m_available_n -= n;
                
                if (m_rev.m_available_n < 0) {
                    m_rev.m_available_n = 0;
                    m_rev.m_ready = false;
                }

            } else if (n == static_cast<ssize_t>(end - last)) {
                
                int nread;

                if (ioctl(m_fd, FIONREAD, &nread) == -1) {
                    n = ERROR;
                    int err = errno;
                    debug_point();
                    break;
                }
                m_rev.m_available_n = nread;
            }

            if (n < static_cast<ssize_t>(end - last)) {

                if (!m_rev.m_pending_eof) {

                    m_rev.m_ready = false;
                }
                m_rev.m_available_n = 0;
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

    m_rev.m_ready = false;

    if (n == ERROR)
    {
        m_rev.m_err = true;
    }

    return n;
}
ssize_t Connection::recv_to_buffer(Buffer *buf)
{
    return recv_to_buffer(buf->m_last, buf->m_end);
}

HttpRequest *Connection::create_http_request(){
    
    Pool *pool = new Pool;

    if (pool == nullptr) {
        return nullptr;
    }

    void *addr = ::calloc(1, sizeof(HttpRequest));
    if (!addr) {
        delete pool;
        return nullptr;
    }

    HttpRequest *r = new (addr) HttpRequest(pool);
    if (!r) {
        free(addr);
        delete pool;
        return nullptr;
    }

    r->m_http_connection = m_data.hc;
    r->m_connection = this;
    r->set_read_event_handler(&HttpRequest::block_reading_handler);
    r->m_write_event_handler = nullptr;

    r->m_header_in_buffer = m_data.hc->m_busy_buffers ? m_data.hc->m_busy_buffers->m_buffer : m_small_buffer;
    r->m_main_request = r;
    r->m_count = 1;

    r->m_start_sec = g_timer->cached_time_sec();
    r->m_start_msec = g_timer->cached_time_msec();

    r->m_method = HTTP_METHOD_UNKNOWN;
    r->m_http_version = 1000;

    r->m_header_out.m_content_length_n = -1;
    r->m_header_out.m_last_modified_time = -1;

    // r->uri_changes = NGX_HTTP_MAX_URI_CHANGES + 1;
    // r->subrequests = NGX_HTTP_MAX_SUBREQUESTS + 1;

    // ngx_http_alloc_request

    ++m_request_cnt;
    return r;
}

BufferChain *Connection::sendfile_from_buffer_chain(BufferChain *in, off_t limit)
{ // ngx_linux_sendfile_chain

    if (!m_wev.m_ready) {
        // 不可写
        return in;
    }

    if (limit == 0 || limit > static_cast<off_t>(MAX_SENDFILE_SIZE - g_pagesize))
    {
        limit = MAX_SENDFILE_SIZE - g_pagesize;
    }


    iovec headers[IOVS_PREALLOCATE];
    IOVector io_vector(headers, IOVS_PREALLOCATE);

    off_t aim_sent = 0, prev_aim_sent;

    for ( ;; ) {
        size_t real_sent = 0;

        prev_aim_sent = aim_sent;
        BufferChain *chain_last = io_vector.fill_with_output_chain(*in, limit - prev_aim_sent);

        if (chain_last == ERROR_ADDR(BufferChain)) {
            return chain_last;
        }

        aim_sent += io_vector.total_length();

        if (m_tcp_nopush == TCP_NOPUSH_UNSET 
            && io_vector.size() != 0                            /* 内存中含有数据 */
            && chain_last 
            && chain_last->m_buffer->m_in_file)                 /* 最后一个未处理的缓冲区在文件中 */
        {
            /* 应该关闭nodelay，即启用nagle算法， 尝试将 m_tcp_nodelay 设置为UNSET，
             * 将 tcp_nopush 设置为 SET */
            if (m_tcp_nodelay == TCP_NODELAY_SET) { 
                int on = 0;
                if (setsockopt(m_fd, IPPROTO_TCP, TCP_NODELAY,
                        &on, sizeof(int)) == -1) 
                {
                    int err = errno;
                    /*
                     * there is a tiny chance to be interrupted, however,
                     * we continue a processing with the TCP_NODELAY
                     * and without the TCP_CORK
                     */
                    if (err != EINTR) {
                        debug_point();
                        m_wev.m_err = true;
                        return ERROR_ADDR(BufferChain);
                    }

                } else {
                    m_tcp_nodelay = TCP_NODELAY_UNSET;
                }
            }
            
            if (m_tcp_nodelay == TCP_NODELAY_UNSET) {
                int on = 1;
                if (setsockopt(m_fd, IPPROTO_TCP, TCP_CORK,
                        &on, sizeof(int)) == -1) 
                {
                    int err = errno;
                    if (err != EINTR) {
                        debug_point();
                        m_wev.m_err = true;
                        return ERROR_ADDR(BufferChain);
                    }

                } else {
                    m_tcp_nopush = TCP_NOPUSH_SET;
                } 
            }
        }

        ssize_t n;
        if (io_vector.size() == 0 && chain_last && chain_last->m_buffer->m_in_file && aim_sent < limit) 
        {
            Buffer *buf = chain_last->m_buffer;
            /* 没有内存中的内容，纯文件，*/
            off_t file_size;
            chain_last = chain_last->coalesce_file(limit - aim_sent, file_size);

            aim_sent += file_size;

            if (file_size == 0) {
                return ERROR_ADDR(BufferChain);
            }

            n = sendfile_from_buffer(buf, file_size);

            if (n == ERROR) {
                return ERROR_ADDR(BufferChain);
            }

            // if (n == NGX_DONE) {
            real_sent = (n == AGAIN) ? 0 : n;

        } else {
            n = write_from_iovec(&io_vector);
            if (n == ERROR) {
                return ERROR_ADDR(BufferChain);
            }
            real_sent = (n == AGAIN) ? 0 : n;
        }

        m_sent += real_sent;

        if (in) {
            in = in->update_sent(real_sent);
        }

        if (n == AGAIN) {
            m_wev.m_ready = false;
            return in;
        }

        if ((size_t)(aim_sent - prev_aim_sent) != real_sent) {
            aim_sent = prev_aim_sent + real_sent;
            continue;
        }
        if (aim_sent >= limit || in == nullptr) {
            return in;
        }
    }
}

void Connection::close_http_request(Int rc) // ngx_http_close_request
{
    if (m_data.r->m_count == 0) {
        // TODO
    }

    m_data.r->m_count--;

    if (m_data.r->m_count || m_data.r->m_blocked)  {
        return;
    }

    free_http_request(rc);
    close_connection();
}

void Connection::finalize_http_request(HttpRequest *r, Int rc)  // ngx_http_finalize_request
{
    if (rc == DONE) {
        finalize_http_connection();
        return;
    }

    if (rc == OK && r->m_filter_finalize) {
        m_error = true;
    }

    if (rc == DECLINED) {
        r->set_write_event_handler(&HttpRequest::run_phases_handler);
        r->run_write_handler();
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

        terminate_http_request(rc);
        return;
    }

    if (rc >= SPECIAL_RESPONSE
        || rc == CREATED
        || rc == NO_CONTENT)
    {
        if (rc == CLOSE) {
            m_timedout = true;
            terminate_http_request(rc);
            return;
        }

        if (r == r->m_main_request) {
            if (m_rev.m_timer_set) {
                g_epoller->del_timer(&m_rev);
            }

            if (m_wev.m_timer_set) {
                g_epoller->del_timer(&m_wev);
            }

            m_rev.set_handler(&Event::http_request_handler);
            m_wev.set_handler(&Event::http_request_handler);
            debug_point();
            // ngx_http_finalize_request(r, ngx_http_special_response_handler(r, rc));
            return;
        }
    }

    if (r != r->m_main_request) {
        debug_point();
    }

    if (r->m_buffered || m_buffered /*|| r->postponed*/) {
        
        if (r->set_write_handler() != OK) {
            r->m_connection->terminate_http_request(0);
        }
        return;
    }

    if (r != m_data.r) {
        debug_point();
        return;
    }

    m_data.r->m_done = true;
    
    m_data.r->set_read_event_handler(&HttpRequest::block_reading_handler);
    m_data.r->set_write_event_handler(&HttpRequest::empty_handler);

    if (m_data.r->m_post_action == false) {
        m_data.r->m_request_complete = true;
    }

    // if (ngx_http_post_action(r) == NGX_OK) {
    //     return;
    // }

    if (m_rev.m_timer_set) {
        g_epoller->del_timer(&m_rev);
    }

    if (m_wev.m_timer_set) {
        m_wev.m_delayed = false;
        g_epoller->del_timer(&m_wev);
    }

    finalize_http_connection();
}

void Connection::free_http_request(Int rc)
{
    // if (m_data.r->m_pool == nullptr) {
    //     debug_point();
    //     return;
    // }
    if (m_data.r == nullptr) {
        debug_point();
        return;
    }

    if (rc > 0 && (m_data.r->m_header_out.m_status == 0 || m_sent == 0)) {
        m_data.r->m_header_out.m_status = rc;
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
                debug_point();
            }
        }
    }

    m_data.r->m_request_line.m_len = 0;
    m_destroyed = true;

    delete m_data.r;
    m_data.r = nullptr;
}


void Connection::terminate_http_request(Int rc) // ngx_http_terminate_request
{
    if (rc > 0 && (m_data.r->m_header_out.m_status == 0 ||  m_sent == 0)) {
        m_data.r->m_header_out.m_status = rc;
    }

    if (m_data.r && m_data.r->m_write_event_handler) {
        if (m_data.r->m_blocked) {
            m_error = true;
            m_data.r->set_write_event_handler(&HttpRequest::http_request_finalizer);
            return;
        }

        m_data.r->set_write_event_handler(&HttpRequest::http_terminate_handler);
        m_data.r->post_http_request();
        // TODO
    }

    close_http_request(rc);
}

void Connection::run_posted_http_requests()
{
    // 当前不支持subrequest
    if (m_destroyed) {
        return;
    }

    if (m_data.r->m_posted) {
        m_data.r->run_write_handler();
        if (m_data.r) {
            m_data.r->m_posted = false;
        }
    }
}

void Connection::set_http_keepalive()
{
    Buffer *b = m_data.r->m_header_in_buffer;
    HttpConnection *hc = m_data.r->m_http_connection;

    if (b->m_pos < b->m_last) {
        /* the pipelined request */
        if (b != m_small_buffer) {
            
            BufferChain *this_bc = nullptr;
            for (BufferChain *bc = hc->m_busy_buffers; bc; )
            {
                if (bc->m_buffer == b) {
                    this_bc = bc;
                    bc = bc->m_next;
                    this_bc->m_next = nullptr;
                    continue;
                }

                Buffer *f = bc->m_buffer;
                f->m_last = f->m_start;
                f->m_pos = f->m_start;

                BufferChain *next = bc->m_next;

                bc->m_next = hc->m_free_buffers;
                hc->m_free_buffers = bc;

                bc = next;
            }

            hc->m_busy_buffers = this_bc;
            hc->m_busy_buffers_num = 1;
        }
    }

    m_data.r->m_keepalive = false;
    free_http_request(0);
    m_data.hc = hc;

    if (!m_rev.m_active && !m_rev.m_ready) {
        if (g_epoller->add_read_event(&m_rev, EPOLLET) != OK) {
            close_http_connection();
            return;
        }
    }

    m_wev.set_handler(&Event::empty_handler);

    if (b->m_pos < b->m_last) {
        LOG_ERROR(LogLevel::error, "浏览器开启 pipeline\n"
                        " ==== %s %d", __FILE__, __LINE__);                   

        HttpRequest *r = create_http_request();
        if (r == nullptr) {
            close_http_connection();
            return;
        }

        r->m_pipeline = true;

        m_data.r = r;
        m_sent = 0;

        m_destroyed = false;

        if (m_rev.m_timer_set) {
            g_epoller->del_timer(&m_rev);
        }

        m_rev.set_handler(&Event::process_http_request_line_handler);
        g_epoller->add_posted_event(&m_rev);
        return;
    }

    /* 释放小的buffer 和 大的 buffer */
    b = m_small_buffer;
    m_pool->free(b->m_start);

    /* the special note for ngx_http_keepalive_handler() that
     * c->buffer's memory was freed */
    b->m_pos = nullptr;

    for(BufferChain *bc = hc->m_free_buffers; bc; /* void */) {
        BufferChain *next = bc->m_next;
        m_pool->free(bc->m_buffer->m_start);
        m_pool->free(bc);

        bc = next;
    }
    hc->m_free_buffers = nullptr;

    for(BufferChain *bc = hc->m_busy_buffers; bc; /* void */) {
        BufferChain *next = bc->m_next;
        m_pool->free(bc->m_buffer->m_start);
        m_pool->free(bc);

        bc = next;
    }
    hc->m_busy_buffers = nullptr;
    hc->m_busy_buffers_num = 0;

    m_rev.set_handler(&Event::http_keepalive_handler);

    if (m_wev.m_active) {
        if (g_epoller->del_write_event(&m_wev, false) != OK) {
            close_http_connection();
            return;
        }
    }
    
    int tcp_nodelay;
    if (m_tcp_nopush == TCP_NOPUSH_SET) {
        int cork = 0;
        if (setsockopt(m_fd, IPPROTO_TCP, TCP_CORK,
                        &cork, sizeof(int)) == -1)
        {
            debug_point();
            close_http_connection();
            return;
        }

        m_tcp_nopush = TCP_NOPUSH_UNSET;
        tcp_nodelay = 1;
    
    } else {
        tcp_nodelay = 0;
    }

    if (tcp_nodelay) {
        if (m_tcp_nodelay == TCP_NODELAY_UNSET) {
            if (setsockopt(m_fd, IPPROTO_TCP, TCP_NODELAY,
                            &tcp_nodelay, sizeof(int)) == -1)
            {
                debug_point();
                close_http_connection();
                return;
            }

            m_tcp_nodelay = TCP_NODELAY_SET;
        }
    }

    m_idle = true;

    g_connection_pool->reusable_connection(this, true);
    g_epoller->add_timer(&m_rev, KEEPALIVE_TIMEOUT);

    if (m_rev.m_ready) {
        g_epoller->add_posted_event(&m_rev);
    }

}

void Connection::set_http_lingering_close()
{
    if (m_data.r->m_lingering_time == 0) {
        m_data.r->m_lingering_time = 
                g_timer->cached_time_sec() + (time_t)(HTTP_LINGERING_TIME / 1000);
    }

    m_rev.set_handler(&Event::http_lingering_close_handler);
    if (!m_rev.m_active && !m_rev.m_ready) {
        if (g_epoller->add_read_event(&m_rev, EPOLLET) != OK) {
            close_http_request(0);
            return;
        }
    }

    m_wev.set_handler(&Event::empty_handler);
    if (m_wev.m_active) {
        if (g_epoller->del_write_event(&m_wev, false) != OK) {
            close_http_request(0);
            return;
        }
    }

    if (shutdown(m_fd, SHUT_WR) == -1) {
        debug_point();
        close_http_request(0);
        return;
    }

    m_close = false;
    g_connection_pool->reusable_connection(this, true);

    g_epoller->add_timer(&m_rev, HTTP_LINGERING_TIMEOUT);

    if (m_rev.m_ready) {
        m_rev.run_handler();
    }
}

ssize_t Connection::sendfile_from_buffer(Buffer *buf, size_t size)
{
    ssize_t n;
    off_t offset = buf->m_file_pos;

    eintr:
    n = sendfile(m_fd, buf->fd(), &offset, size);
    
    if (n == -1)
    {   
        int err = errno;
        switch (err) 
        {
        case EAGAIN:
            return AGAIN;
        
        case EINTR:
            goto eintr;

        default:
            m_wev.m_err = true;
            debug_point();
            return ERROR;
        }
    }

    if (n == 0) {
        /*
         * if sendfile returns zero, then someone has truncated the file,
         * so the offset became beyond the end of the file
         */
        LOG_ERROR(LogLevel::info, "(%s: %d) sendfile() failed, file was truncated\n", __FILE__, __LINE__);
        return ERROR;
    }
    return n;
}

ssize_t Connection::write_from_iovec(IOVector *iov)
{
    ssize_t n;

eintr:
    n = writev(m_fd, iov->data(), iov->size());

    if (n == -1)
    {   
        int err = errno;
        switch (err) 
        {
        case EAGAIN:
            return AGAIN;
        
        case EINTR:
            goto eintr;

        default: 
            m_wev.m_err = true;
            debug_point();
            return ERROR;
        }
    }
    return n;
}

void *Connection::pool_malloc(size_t s, Deleter deleter) {
    return m_pool->malloc(s, deleter);
}

void *Connection::pool_calloc(size_t n, size_t size, Deleter deleter) {
    return m_pool->calloc(n, size, deleter);
}

void Connection::pool_free(void *addr) {
    m_pool->free(addr);
}