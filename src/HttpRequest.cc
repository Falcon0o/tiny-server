#include "HttpRequest.h"

#include "Connection.h"
#include "HttpConnection.h"
#include "Buffer.h"
#include "BufferChain.h"
#include "Epoller.h"
#include "Event.h"
#include "File.h"
#include "OpenFileInfo.h"
#include "CachedOpenFile.h"
HttpRequest::HttpRequest() 
:   m_phase_engine(this) {}

void HttpRequest::block_reading_handler()
{
    Event *rev = m_connection->m_read_event;
    
    if (rev->m_active) {
        if (g_epoller->del_read_event(rev, false) != OK)
        {
            m_connection->close_http_request(0);
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

        if (m_uri_end) {
            m_uri_end   = b->m_start + (m_uri_end       - old_start);
        }

        if (m_uri_ext_start) {
            m_uri_ext_start   = b->m_start + (m_uri_ext_start       - old_start);
        }
        
        if (m_uri_ext_end) {
            m_uri_ext_end     = b->m_start + (m_uri_ext_end       - old_start);
        }

        if (m_http_protocol_start) {
            m_http_protocol_start = b->m_start + (m_http_protocol_start - old_start);
        }
    
    } else {
        
        m_header_name_start = b->m_start;
        if (m_header_name_end) {
            m_header_name_end = b->m_start + (m_header_name_end - old_start);
        }
        if (m_header_start) {
            m_header_start = b->m_start + (m_header_start - old_start);
        }
        if (m_header_end) {
            m_header_end = b->m_start + (m_header_end - old_start);
        }
        
    } 

    m_header_in_buffer = b;
    return OK;
}

void HttpRequest::http_terminate_handler()
{
    m_count = 1;
    m_connection->close_http_request(0);
}

Int HttpRequest::post_http_request()
{
    m_posted = true;
    return OK;
}

ssize_t HttpRequest::read_request_header()
{
    ssize_t can_read_ssize = m_header_in_buffer->m_last - m_header_in_buffer->m_pos;
    
    if (can_read_ssize > 0) {
        return can_read_ssize;
    }
    
    Event *rev = m_connection->m_read_event;
    if (rev->m_ready) {
        can_read_ssize = m_connection->recv_to_buffer(m_header_in_buffer);
    
    } else {
        can_read_ssize = AGAIN;
    }

    if (can_read_ssize == AGAIN) {
        if (!rev->m_timer_set) {
            g_epoller->add_timer(rev, CLIENT_HEADER_TIMEOUT);
        }

        if (!rev->m_active && !rev->m_ready) {
            if (g_epoller->add_read_event(rev, EPOLLET) == ERROR) {
                m_connection->close_http_request(INTERNAL_SERVER_ERROR);
                return ERROR;
            }
        }

        return AGAIN;
    }

    if (can_read_ssize == 0) {
        LOG_ERROR(LogLevel::alert, "client prematurely closed connection (%s: %d)\n", __FILE__, __LINE__);
    }

    if (can_read_ssize == 0 || can_read_ssize == ERROR) {
        m_connection->m_error = true;
        m_connection->finalize_http_request(this, BAD_REQUEST);
        return ERROR;
    }

    m_header_in_buffer->m_last += can_read_ssize;
    return can_read_ssize;
}


static unsigned usual[] = {
    0xffffdbfe, /* 1111 1111 1111 1111  1101 1011 1111 1110 */
                /* ?>=< ;:98 7654 3210  /.-, +*)( '&%$ #"!  */

    0x7fff37d6, /* 0111 1111 1111 1111  0011 0111 1101 0110 */
                /* _^]\ [ZYX WVUT SRQP  ONML KJIH GFED CBA@ */

    0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
                /*  ~}| {zyx wvut srqp  onml kjih gfed cba` */

    0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
    0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
    0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
    0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
    0xffffffff  /* 1111 1111 1111 1111  1111 1111 1111 1111 */
};

static bool is_usual(unsigned char c)
{
    return usual[c >> 5] & (1U << (c & 0x1f));
}

Int HttpRequest::parse_request_line()
{
    enum class ParseState{
        start = 0,
        method,
        spaces_before_uri,
        after_slash_in_uri,
        check_uri,
        check_uri_http_09,
        http_H,
        http_HT,
        http_HTT,
        http_HTTP,
        first_major_digit, 
        major_digit,
        first_minor_digit,
        minor_digit,
        spaces_after_digit,
        almost_done
    };

    ParseState parse_state = static_cast<ParseState>(m_parse_state);

    u_char *pos = m_header_in_buffer->m_pos;

    for ( ; pos < m_header_in_buffer->m_last; ++pos)
    {
        u_char downcast;
        switch (parse_state) {
        
        case ParseState::start:
            m_request_start = pos;

            if (*pos == CR || *pos == LF) {
                break;
            }

            if ((*pos < 'A' || *pos > 'Z') && *pos != '_' && *pos != '-') {
                return PARSE_INVALID_METHOD;
            }

            parse_state = ParseState::method;
            break;

        case ParseState::method:
            if (*pos == ' ') {
                m_method_end = pos;

                Int rc = parse_method();
                if (rc != OK) {
                    return rc;
                }

                parse_state = ParseState::spaces_before_uri;
                break;
            }

            if ((*pos < 'A' || *pos > 'Z') && *pos != '_' && *pos != '-') {
                return PARSE_INVALID_METHOD;
            }

            break;

        case ParseState::spaces_before_uri:
            if (*pos == '/') {
                m_uri_start = pos;
                parse_state = ParseState::after_slash_in_uri;
                break;
            }

            switch (*pos) 
            {
            case ' ':
                break;
            default:
                return PARSE_INVALID_REQUEST;
            }
            break;

        case ParseState::after_slash_in_uri:
            if (is_usual(*pos)) {
                parse_state = ParseState::check_uri;
                break;
            }

            switch (*pos){
            case ' ':
                m_uri_end = pos;
                parse_state = ParseState::check_uri_http_09;
                break;
            default:
                LOG_ERROR(LogLevel::info, "暂不支持的协议版本 (%s: %d)", __FILE__, __LINE__);
                return PARSE_INVALID_VERSION;
            }
            break;

        case ParseState::check_uri:
            if (is_usual(*pos)) {
                parse_state = ParseState::check_uri;
                break;
            }

            switch (*pos)
            {
            case '/':
                m_uri_ext_start = nullptr;
                parse_state = ParseState::after_slash_in_uri;
                break;
            case '.':
                m_uri_ext_start = pos + 1;
                break;
            case ' ':
                m_uri_end = pos;
                parse_state = ParseState::check_uri_http_09;
                break;

            default:
                return PARSE_INVALID_REQUEST;
            }
            break;

        case ParseState::check_uri_http_09:
            switch (*pos) {
            case ' ':
                break;
            case CR:
            case LF:
                LOG_ERROR(LogLevel::info, "暂不支持的协议版本 (%s: %d)", __FILE__, __LINE__);
                return PARSE_INVALID_VERSION;
            case 'H':
                m_http_protocol_start = pos;
                parse_state = ParseState::http_H;
                break;
            default:
                m_space_in_uri = true;
                parse_state = ParseState::check_uri;
                --pos;
            }
            break;

        case ParseState::http_H:
            switch (*pos)
            {
            case 'T':
                parse_state = ParseState::http_HT;
                break;
            default:
                return PARSE_INVALID_REQUEST;
            }
            break;

        case ParseState::http_HT:
            switch (*pos)
            {
            case 'T':
                parse_state = ParseState::http_HTT;
                break;
            default:
                return PARSE_INVALID_REQUEST;
            }
            break;

        case ParseState::http_HTT:
            switch (*pos)
            {
            case 'P':
                parse_state = ParseState::http_HTTP;
                break;
            default:
                return PARSE_INVALID_REQUEST;
            }
            break;
    
        case ParseState::http_HTTP:
            switch (*pos)
            {
            case '/':
                parse_state = ParseState::first_major_digit;
                break;
            default:
                return PARSE_INVALID_REQUEST;
            }
            break;

        case ParseState::first_major_digit:
            if (*pos < '1' || *pos > '9') {
                return PARSE_INVALID_REQUEST;
            }

            m_http_major = *pos - '0';
            if (m_http_major > 1) {
                return PARSE_INVALID_VERSION;
            }
            
            parse_state = ParseState::major_digit;
            break;
        
        case ParseState::major_digit:
            if (*pos == '.') {
                parse_state = ParseState::first_minor_digit;
                break;
            }

            if (*pos < '0' || *pos > '9') {
                return PARSE_INVALID_REQUEST;
            }

            m_http_major = m_http_major * 10 + (*pos - '0');
            
            if (m_http_major > 1) {
                return PARSE_INVALID_VERSION;
            }
            break;

        case ParseState::first_minor_digit:
            if (*pos < '0' || *pos > '9') {
                return PARSE_INVALID_REQUEST;
            }

            m_http_minor = *pos - '0';
            parse_state = ParseState::minor_digit;
            break;

        case ParseState::minor_digit:
            if (*pos == CR) {
                parse_state = ParseState::almost_done;
                break; 
            }

            if (*pos == LF) {
                goto done;
            }

            if (*pos == ' ') {
                parse_state = ParseState::spaces_after_digit;
                break;
            }

            if (*pos < '0' || *pos > '9') {
                return PARSE_INVALID_REQUEST;
            }

            if (m_http_minor > 99) {
                return PARSE_INVALID_REQUEST;
            }

            m_http_minor = m_http_minor * 10 + (*pos - '0');
            break;

        case ParseState::spaces_after_digit:
            switch (*pos) {
            case ' ':
                break;
            case CR:
                parse_state = ParseState::almost_done;
                break; 
            case LF:
                goto done;
            default:
                return PARSE_INVALID_REQUEST;
            }
            break;

        case ParseState::almost_done: 
            // *(pos - 1) = CR 
            m_request_end = pos - 1;
            switch (*pos)
            {
            case LF:
                goto done;
            default:
                return PARSE_INVALID_REQUEST;
            }
        }
    }

    m_header_in_buffer->m_pos = pos;
    m_parse_state = static_cast<uInt>(parse_state);

    return AGAIN;

done:
    m_header_in_buffer->m_pos = pos + 1;
    if (m_request_end == nullptr) {
        m_request_end = pos; /* LF 的位置*/
    }

    m_http_version = m_http_major * 1000 + m_http_minor;
    m_parse_state = 0;

    return OK;
}

Int HttpRequest::parse_method()
{
    /* 小端法 */
    unsigned int m = *reinterpret_cast<unsigned int*>(m_request_start);
    switch (m_method_end - m_request_start) {

    case 3:
        if (m == ('G' | ('E' << 8) | ('T' << 16) | (' ' << 24)) )
        {
            m_method = HTTP_METHOD_GET;
            return OK; 
        }
        break;
    }
    return PARSE_INVALID_METHOD;
}


Int HttpRequest::parse_header_line()
{
    static unsigned char lowcase[] =
        "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
        "\0\0\0\0\0\0\0\0\0\0\0\0\0-\0\0" "0123456789\0\0\0\0\0\0"
        "\0abcdefghijklmnopqrstuvwxyz\0\0\0\0\0"
        "\0abcdefghijklmnopqrstuvwxyz\0\0\0\0\0"
        "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
        "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
        "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
        "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";

    enum class ParseState{
        start = 0,
        all_headers_almost_done,
        header_name,
        space_before_header_value,
        header_value,
        one_header_almost_done,
        space_after_header_value
    };

    ParseState parse_state = static_cast<ParseState>(m_parse_state);

    u_char *pos = m_header_in_buffer->m_pos;

    for ( ; pos != m_header_in_buffer->m_last; ++pos) {

        switch (parse_state) {

        case ParseState::start:
            m_header_name_start = pos;
            m_invalid_header = false;

            switch (*pos) {
            case CR:
                m_header_end = pos;
                parse_state = ParseState::all_headers_almost_done;
                break;
            case LF: 
                m_header_end = pos;
                goto all_headers_done;
            default:
                parse_state = ParseState::header_name;

                if (lowcase[*pos]) {
                    m_header_hash = hash(0, lowcase[*pos]);
                    m_lowcase_header[0] = lowcase[*pos];
                    m_lowcase_index = 1;
                    break;
                }

                if (*pos == '_') {
                    if (ALLOW_UNDERSCORES_IN_HEADERS) {
                        m_header_hash = hash(0, *pos);
                        m_lowcase_header[0] = *pos;
                        m_lowcase_index = 1;
                    
                    } else {
                        m_header_hash = 0;
                        m_lowcase_index = 0;
                        m_invalid_header = true;
                    }
                    break;
                } 

                if (*pos == '\0') {
                    return PARSE_INVALID_HEADER;
                }

                m_header_hash = 0;
                m_lowcase_index = 0;
                m_invalid_header = true;
                break;
            }
            break;
        
        case ParseState::header_name:
            if (lowcase[*pos]) {
                m_header_hash = hash(m_header_hash, lowcase[*pos]);
                m_lowcase_header[m_lowcase_index++] = lowcase[*pos];
                m_lowcase_index &= (HTTP_LOWCAST_HEADER_LEN - 1);
                break;
            }
            
            if (*pos == '_') {
                if (ALLOW_UNDERSCORES_IN_HEADERS) {
                    m_header_hash = hash(m_header_hash, *pos);
                    m_lowcase_header[m_lowcase_index++] = *pos;
                    m_lowcase_index &= (HTTP_LOWCAST_HEADER_LEN - 1);

                } else {
                    m_invalid_header = true;
                }
                break;
            }

            if (*pos == ':') {
                m_header_name_end = pos;
                parse_state = ParseState::space_before_header_value;
                break;
            }

            if (*pos == CR) {
                m_header_name_end = pos;
                m_header_start = pos;
                m_header_end = pos;
                parse_state = ParseState::one_header_almost_done;
                break;
            }

            if (*pos == LF) {
                m_header_name_end = pos;
                m_header_start = pos;
                m_header_end = pos;
                goto one_header_done;
                break;
            }

            if (*pos == '\0') {
                return PARSE_INVALID_HEADER;
            }

            m_invalid_header = true;
            break;

        case ParseState::space_before_header_value:
            switch (*pos) {
            case ' ':
                break;
            case CR:
                m_header_start = pos;
                m_header_end = pos;
                parse_state = ParseState::one_header_almost_done;
                break;
            case LF:
                m_header_start = pos;
                m_header_end = pos;
                goto one_header_done;
            case '\0':
                PARSE_INVALID_HEADER;
            default:
                m_header_start = pos;
                parse_state = ParseState::header_value;
                break;
            }
            break;

        case ParseState::header_value:
            switch (*pos) {
            case ' ':
                m_header_end = pos;
                parse_state = ParseState::space_after_header_value;
                break;
            case CR:
                m_header_end = pos;
                parse_state = ParseState::one_header_almost_done;
                break;
            case LF: 
                m_header_end = pos;
                goto one_header_done;
            case '\0':
                return PARSE_INVALID_HEADER;
            }
            break;

        case ParseState::space_after_header_value:
            switch (*pos) {
            case ' ':
                break;
            case CR:
                parse_state = ParseState::one_header_almost_done;
                break;
            case LF:
                goto one_header_done;
            case '\0':
                return PARSE_INVALID_HEADER;
            default:
                parse_state = ParseState::header_value;
                break;
            }
            break;

        case ParseState::one_header_almost_done:
            switch (*pos) {
            case LF:
                goto one_header_done;
            case CR:
                break;
            default:
                return PARSE_INVALID_HEADER;
            }
            break;

        case ParseState::all_headers_almost_done:
            switch (*pos) {
            case LF:
                goto all_headers_done;
            default:
                return PARSE_INVALID_HEADER;
            }
        }
    }

    m_header_in_buffer->m_pos = pos;
    m_parse_state = static_cast<uInt>(parse_state);

    return AGAIN;

one_header_done:
    m_header_in_buffer->m_pos = pos + 1;
    m_parse_state = 0;

    return OK;

all_headers_done:
    m_header_in_buffer->m_pos = pos + 1;
    m_parse_state = 0;

    return PARSE_HEADER_DONE;
}

Int HttpRequest::process_request_uri()
{
    // m_unparsed_uri = m_uri;
    // m_valid_unparsed_uri = true;
    return OK;
}

Int HttpRequest::process_request_header()
{
    if (m_header_in.m_host == m_header_in.m_headers.end()
        && m_http_version > 1000) 
    {
        LOG_ERROR(LogLevel::alert, "client sent HTTP/1.1 request without \"Host\" header (%s: %d)\n", __FILE__, __LINE__);
        m_connection->finalize_http_request(this, BAD_REQUEST);
        return ERROR;
    }

    return OK;
}

void HttpRequest::process_request()
{
    if (m_connection->m_read_event->m_timer_set) {
        g_epoller->del_timer(m_connection->m_read_event);
    }

    m_connection->m_read_event->set_handler(&Event::http_request_handler);
    m_connection->m_write_event->set_handler(&Event::http_request_handler);

    set_read_event_handler(&HttpRequest::block_reading_handler);
    http_handler();
}

void HttpRequest::http_handler()
{
    switch (m_header_in.m_connection_type)
    {
    case 0:
        m_keepalive = m_http_version > 1000;
        break;
    
    case HTTP_CONNECTION_CLOSE:
        m_keepalive = false;
        break;
    
    case HTTP_CONNECTION_KEEPALIVE:
        m_keepalive = true;
        break;
    }

    m_lingering_close = (m_header_in.m_content_length_n > 0
                         || m_header_in.m_chunked);
    
    set_write_event_handler(&HttpRequest::run_phases_handler);
    run_write_handler();
}

void HttpRequest::http_request_finalizer() 
{
    m_connection->finalize_http_request(this, 0);
}



void HttpRequest::run_phases_handler()
{
    for (int i = 0; i < HttpPhaseEngine::checkers.size(); ++i) {
        Int rc = (m_phase_engine.*HttpPhaseEngine::checkers[i])();
        if (rc == OK) {
            return;
        }
    }
    
}


StringSlice HttpRequest::map_uri_to_path() 
{
    size_t path_len = sizeof(PREFIX)- 1 + m_uri.m_len;
    void *addr = m_pool->malloc(path_len + 1);
    if (addr == nullptr) {
        m_connection->close_http_request(INTERNAL_SERVER_ERROR);
    }

    u_char *path_data = static_cast<u_char*>(addr);
    memcpy(path_data, PREFIX, (sizeof(PREFIX) - 1));
    memcpy(path_data + (sizeof(PREFIX) - 1), m_uri.m_data, m_uri.m_len);
    
    path_data[path_len] = '\0';
    return StringSlice(path_data, path_len);
}


Int HttpRequest::http_static_handler()
{
    if (!(m_method & (HTTP_METHOD_GET|HTTP_METHOD_POST|HTTP_METHOD_HEAD))) {
        return NOT_ALLOWED;
    }

    if (m_uri.m_data[m_uri.m_len - 1] == '/') {
        return DECLINED;
    }

    StringSlice &&path = map_uri_to_path();
    LOG_ERROR(LogLevel::info, "`%s\'\n", path.m_data);

    OpenFileInfo info;
    if (CachedOpenFile::open_cached_file(path, info) == ERROR) {
        return INTERNAL_SERVER_ERROR;
    }

    if (!info.m_is_file) {
        return NOT_FOUND;
    }

    if (m_method == HTTP_METHOD_POST) {
        return NOT_ALLOWED;
    }

    Int rc = discard_request_body();

    if (rc != OK) {
        return rc;
    }

    m_header_out.m_status               = HTTP_OK;
    m_header_out.m_content_length_n     = info.m_size;
    m_header_out.m_last_modified_time   = info.m_mtime;

    if (m_header_out.m_content_type.m_len == 0) {
        if (m_uri_ext.m_len == 0) {
            m_header_out.m_content_type = DEFAULT_CONTEXT_TYPE;
        
        } else {
            auto iter = m_header_out.s_content_types.find(m_uri_ext.hash());
            if (iter != m_header_out.s_content_types.end()) {
                m_header_out.m_content_type = iter->second;
                
            } else {
                m_header_out.m_content_type = DEFAULT_CONTEXT_TYPE;
            }

        }
    }

    m_allow_ranges = true;

    

    rc = response_header_filter();
    if (rc == ERROR || rc > OK || m_header_only) {
        return rc;
    }

    Buffer *file_buf = Buffer::create_file_buffer(m_pool, info);
    if (file_buf == nullptr) {
        return INTERNAL_SERVER_ERROR;
    }

    file_buf->m_last_buf = (this == m_main_request);
    file_buf->m_last_buf_in_chain = true;
    file_buf->m_file->m_name = path;

    BufferChain out(file_buf);
    return response_body_filter(&out);
}


Int HttpRequest::response_header_filter() 
{
    if (m_header_sent) {
        return OK;
    }

    m_header_sent = true;

    if (m_header_out.m_last_modified_time != -1) {
        if (m_header_out.m_status != HTTP_OK
            && m_header_out.m_status != PARTIAL_CONTENT
            && m_header_out.m_status != NOT_MODIFIED)
        {
            m_header_out.m_last_modified_time = -1;
            // TODO m_header_out.m_last_modified = nullptr;
        }
    }

    size_t len = sizeof("HTTP/1.x ") - 1 + sizeof(CRLF) - 1 + sizeof(CRLF) - 1;

    StringSlice status_line;

    if (m_header_out.m_status_line.m_len) {
        status_line = m_header_out.m_status_line;
        

    } else {
        status_line = GetHttpStatusLines(m_header_out.m_status);
    }

    len += status_line.m_len;
    len += sizeof("Date: Mon, 28 Sep 1970 06:00:00 GMT" CRLF) - 1;

    if (m_header_out.m_content_type.m_len) {
        len += sizeof("Content-Type: ") - 1 
            + m_header_out.m_content_type.m_len + 2;
    }

    if (m_header_out.m_content_length_n >= 0) {
        len += sizeof("Content-Length: ") - 1 
            + MAX_OFF_T_LEN + 2;
    }

    if (m_header_out.m_last_modified_time != -1) {
        len += sizeof("Last-Modified: Mon, 28 Sep 1970 06:00:00 GMT" CRLF) - 1;
    }

    if (m_keepalive) {
        len += sizeof("Connection: keep-alive" CRLF) - 1;
        
        if (KEEPALIVE_TIMEOUT) {
            len += sizeof("Keep-Alive: timeout=") - 1 
                + MAX_TIME_T_LEN + 2;
        }
    } else {
        len += sizeof("Connection: close" CRLF) - 1;
    }

    Buffer *buf = Buffer::create_temp_buffer(m_pool, len);

    if (buf == nullptr) {
        return ERROR;
    }

    *buf << STRING_SLICE("HTTP/1.1 ") << status_line << B_CRLF();

    if (m_header_out.m_content_type.m_len) {
        *buf << STRING_SLICE("Content-Type: ") << m_header_out.m_content_type << B_CRLF();
    }

    if (m_header_out.m_content_length_n >= 0) {
        *buf << STRING_SLICE("Content-Length: ") 
             << OFF_T(m_header_out.m_content_length_n)
             << B_CRLF();
    }

    if (m_header_out.m_last_modified_time != -1) {
        *buf << STRING_SLICE("Last-Modified: ")
             << TIME_T(m_header_out.m_last_modified_time)
             << B_CRLF();
    }

    if (m_keepalive) {
        *buf << STRING_SLICE("Connection: keep-alive" CRLF);

        if (KEEPALIVE_TIMEOUT) {
            *buf << STRING_SLICE("Keep-Alive: timeout=")
                 << MSEC_T(KEEPALIVE_TIMEOUT)
                 << B_CRLF();
        }
    } else {
        *buf << STRING_SLICE("Connection: close" CRLF);
    }

    *buf << B_CRLF();
    
    BufferChain out(buf);
    return response_body_filter(&out);
}

Int HttpRequest::response_body_filter(BufferChain *in)
{

    BufferChain **out = &m_out_buffer_chain;

    for (BufferChain *bc = m_out_buffer_chain; bc; bc = bc->m_next) {
        out = &bc->m_next;
    }
    
    for (; in; in = in->m_next) {
        *out = m_pool->alloc_buffer_chain();
        (*out)->m_buffer = in->m_buffer;        
        out = &(*out)->m_next;
    }
    *out = nullptr;

    BufferChain *chain_last = 
        m_connection->sendfile_from_buffer_chain(m_out_buffer_chain, SENDFILE_MAX_CHUNK);

    if (chain_last == ERROR_ADDR(BufferChain)) {
        m_connection->m_error = true;
        return ERROR;
    }

    for (BufferChain *bc = m_out_buffer_chain; bc != chain_last; )
    {
        BufferChain *prev = bc;
        bc = bc->m_next;
    }

    m_out_buffer_chain = chain_last;

    if (chain_last) {
        m_connection->m_buffered |= HTTP_WRITE_BUFFERED;
        return AGAIN;
    }

    m_connection->m_buffered &= ~HTTP_WRITE_BUFFERED;

    return OK;
}

void HttpRequest::discarded_request_body_handler()
{
    LOG_ERROR(LogLevel::info, "未定义的空函数 (%s: %d)", __FILE__, __LINE__);
}

Int HttpRequest::discard_request_body()
{
    LOG_ERROR(LogLevel::info, "未定义的空函数 (%s: %d)", __FILE__, __LINE__);
    return OK;
}

void HttpRequest::http_test_reading()
{
    LOG_ERROR(LogLevel::info, "未定义的空函数 (%s: %d)", __FILE__, __LINE__);
}

Int HttpRequest::set_write_handler()
{
    if (m_discard_body) {
        set_read_event_handler(&HttpRequest::discarded_request_body_handler);
    } else {
        set_read_event_handler(&HttpRequest::http_test_reading);
    }

    set_write_event_handler(&HttpRequest::http_writer);

    Event *wev = m_connection->m_write_event;
    if (wev->m_ready || wev->m_delayed) {
        return OK;
    }

    if (!wev->m_delayed) {
        g_epoller->add_timer(wev, SEND_TIMEOUT);
    }

    if (!wev->m_active && !wev->m_ready) {
        if (g_epoller->add_write_event(wev, EPOLLET) == ERROR) {
            m_connection->close_http_request(INTERNAL_SERVER_ERROR);
            return ERROR;
        }
    }
    return OK;
}

void HttpRequest::http_writer()
{
    Connection *c = m_connection;
    Event *wev = c->m_write_event;

    if (wev->m_timeout) {
        c->m_timedout = true;
        c->finalize_http_request(this, REQUEST_TIME_OUT);
        return;
    }

    if (m_aio || wev->m_delayed) {
        if (!wev->m_delayed) {
            g_epoller->add_timer(wev, SEND_TIMEOUT);
        }
        if (!wev->m_active && !wev->m_ready) {
            if (g_epoller->add_read_event(wev, EPOLLET) == ERROR) {
                c->close_http_request(0);
            }
        }
        return;
    }

    Int rc = response_body_filter(nullptr);

    if (rc == ERROR) {
        c->finalize_http_request(this, rc);
        return;
    }

    if (this == m_main_request && c->m_buffered) {
        if (!wev->m_delayed) {
            g_epoller->add_timer(wev, SEND_TIMEOUT);
        }
        if (!wev->m_active && !wev->m_ready) {
            if (g_epoller->add_read_event(wev, EPOLLET) == ERROR) {
                c->close_http_request(0);
            }
        }
        return;
    }

    set_write_event_handler(&HttpRequest::empty_handler);

    c->finalize_http_request(this, rc);
}
