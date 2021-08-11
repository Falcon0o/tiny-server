#include "IOVector.h"

#include <sys/uio.h>

#include "Buffer.h"
#include "BufferChain.h"

iovec &IOVector::operator[](size_t i)
{
    return m_array[i];
}

iovec *IOVector::emplace_back(void *base, size_t len)
{
    if (m_size > 0 && (u_char*)m_array[m_size - 1].iov_base + m_array[m_size - 1].iov_len == base) {
        m_array[m_size - 1].iov_len += len;
        m_len_sum += len;
        return m_array + m_size - 1;
    }

    if (size() == capacity()) {
        return nullptr;
    }

    m_array[m_size].iov_base = base;
    m_array[m_size].iov_len = len;
    ++m_size;
    m_len_sum += len;
    return m_array + m_size - 1;
}


// TODO 验证 const_cast
BufferChain *IOVector::fill_with_output_chain(BufferChain &in, size_t limit)
{
    m_len_sum = 0;
    m_size = 0;
    BufferChain *curr_chain = &in;
    
    while (curr_chain && m_len_sum < limit) 
    {
        Buffer *curr_buf = curr_chain->m_buffer;

        if (curr_buf->special()) {
            continue;
        }

        if (curr_buf->m_in_file) {
            break;
        }

        if (!curr_buf->in_memory()) {
            // debug_point();
            return ERROR_ADDR(BufferChain);
        }

        size_t curr_size = curr_buf->m_last - curr_buf->m_pos;
        if (curr_size > limit - m_len_sum) {
            curr_size = limit - m_len_sum;
        }

        if (!emplace_back(curr_buf->m_pos, curr_size)) {
            return curr_chain;
        }
        curr_chain = curr_chain->m_next;
    }

    return curr_chain;
}