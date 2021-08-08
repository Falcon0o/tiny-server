#include "HttpRequest.h"

#include "Connection.h"
#include "HttpConnection.h"
#include "Buffer.h"
#include "BufferChain.h"
#include "Epoller.h"
#include "Event.h"


void HttpRequest::block_reading_handler()
{
    Event *rev = m_connection->m_read_event;
    
    if (rev->m_active) {
        if (g_epoller->del_read_event(rev, false) != OK)
        {
            m_connection->close_http_request(this, 0);
        }
    }
}

bool HttpRequest::need_alloc_large_header_buffer() const
{
    return m_header_in_buffer->m_pos == m_header_in_buffer->m_end;
}

Int HttpRequest::alloc_large_header_buffer(bool request_line)
{
    if (m_parse_state == 0 && request_line)
    {
        /* the client fills up the buffer with "\r\n" */
        m_header_in_buffer->m_pos = m_header_in_buffer->m_start;
        m_header_in_buffer->m_last = m_header_in_buffer->m_start;
        return OK;
    }

    u_char *old_start = request_line ? m_request_start : m_header_name_start;

    if (m_parse_state != 0 
        && (size_t)(m_header_in_buffer->m_pos - old_start) > LARGE_CLIENT_HEADER_BUFFER_SIZE)
    {
        return DECLINED;
    }

    Buffer *b = nullptr;
    BufferChain *bc = nullptr;

    if (m_http_connection->m_free_buffers) {
        bc = m_http_connection->m_free_buffers;
        b = bc->m_buffer;
        m_http_connection->m_free_buffers = bc->m_next;
    
    } else if (m_http_connection->m_busy_buffers_num < LARGE_CLIENT_HEADER_BUFFER_NUM){
        b = Buffer::create_temp_buffer(&m_connection->m_pool, LARGE_CLIENT_HEADER_BUFFER_SIZE);

        if (b == nullptr) {
            return ERROR;
        }

        BufferChain *bc = static_cast<BufferChain*>(m_connection->m_pool.malloc(sizeof(BufferChain)));
        if (bc == nullptr) {
            return ERROR;
        }
        bc->m_buffer = b;
    } else {
        return DECLINED;
    }

    bc->m_next = m_http_connection->m_busy_buffers;
    m_http_connection->m_busy_buffers = bc;
    ++m_http_connection->m_busy_buffers_num;

    if (m_parse_state == 0) {
        /*
         * r->state == 0 means that a header line was parsed successfully
         * and we do not need to copy incomplete header line and
         * to relocate the parser header pointers
         */
        m_header_in_buffer = b;
        return OK;
    }

    if (m_header_in_buffer->m_pos - old_start > b->m_end - b->m_start){
        return ERROR;
    }

    memcpy(b->m_start, old_start, m_header_in_buffer->m_pos - old_start);
    b->m_pos = b->m_start + (m_header_in_buffer->m_pos - old_start);
    b->m_last = b->m_pos;

    if (request_line) 
    {
        m_request_start = b->m_start;

        if (m_request_end) {
            m_request_end = b->m_start + (m_request_end - old_start);
        }

        m_method_end    = b->m_start + (m_method_end    - old_start);
        m_uri_start     = b->m_start + (m_uri_start     - old_start);
        m_uri_end       = b->m_start + (m_uri_end       - old_start);
        
        if (m_http_protocol_start) {
            m_http_protocol_start = b->m_start + (m_http_protocol_start - old_start);
        }
    
    } else {
        m_header_name_start     = b->m_start;
        m_header_name_end       = b->m_start + (m_header_name_end   - old_start);
        m_header_start          = b->m_start + (m_header_start      - old_start);
        m_header_end            = b->m_start + (m_header_end        - old_start);
    } 

    m_header_in_buffer = b;
    return OK;
}
