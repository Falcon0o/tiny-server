#include "HttpConnection.h"

// #include "Buffer.h"

#include <algorithm>
HttpConnection::HttpConnection()
:   m_busy_buffers_num(0),
    m_busy_buffers(nullptr),
    m_free_buffers(nullptr)
{

}

HttpConnection::~HttpConnection()
{
    
}

// Buffer *HttpConnection::get_busy_buffer() const
// {
//     if (m_busy_buffers.empty()) {
//         return nullptr;
//     }

//     return *m_busy_buffers.begin();
// }

// Buffer *HttpConnection::get_or_create_busy_buffer()
// {
//     Buffer *buf = nullptr;
//     if (!m_free_buffers.empty()) {
//         buf = *m_free_buffers.begin();
//         m_free_buffers.erase_after(m_free_buffers.before_begin());
//         buf->set_pos(buf->start());
//         buf->set_last(buf->start());

//     } else if (m_busy_buffers_num < LARGE_CLIENT_HEADER_BUFFER_NUM) {
//         buf = Buffer::create_temp_buffer(LARGE_CLIENT_HEADER_BUFFER_SIZE);

//         if (!buf) {
//             return ERROR_ADDR(Buffer);
//         }

//     } else {
//         return DECLINED_ADDR(Buffer);
//     }

//     ++m_busy_buffers_num;
//     m_busy_buffers.push_front(buf);
//     return buf;
// }


// void HttpConnection::move_all_to_free_buffers_but_this(Buffer *buf)
// {
//     auto iter = std::find(m_busy_buffers.begin(), m_busy_buffers.end(), buf);

//     if (iter != m_busy_buffers.end() && iter._M_next() != m_busy_buffers.end()) {
//         m_free_buffers.splice_after(m_free_buffers.before_begin(),
//                                     m_busy_buffers, iter, m_busy_buffers.end());
//     }

//     if (iter != m_busy_buffers.begin())
//     {
//         m_free_buffers.splice_after(m_free_buffers.before_begin(),
//                                     m_busy_buffers, m_busy_buffers.before_begin(), iter);
//     }

//     m_busy_buffers_num = (iter == m_busy_buffers.end()) ? 0 : 1;
// }


//     Buffer *buf = *m_free_buffers.begin();
//     m_free_buffers.erase_after(m_free_buffers.before_begin());

//     return buf;
// }

// Buffer *HttpConnection::push_front_busy_buffer()
// {
//     if (m_busy_buffers_num >= LARGE_CLIENT_HEADER_BUFFER_NUM) {
//         return nullptr;
//     }

//     Buffer *buf = Buffer::create_temp_buffer(LARGE_CLIENT_HEADER_BUFFER_SIZE);

//     if (!buf) {
//         return nullptr;
//     }

// }