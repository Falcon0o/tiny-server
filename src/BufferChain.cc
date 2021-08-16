#include "BufferChain.h"

#include "Buffer.h"
#include "Cycle.h"


BufferChain *BufferChain::coalesce_file(off_t limit, off_t &total)
{
    
    total = 0;

    off_t prev_last;
    BufferChain *curr_chain = this;
    do {
        Buffer *curr_buf = curr_chain->m_buffer;

        off_t size = curr_buf->m_file_last - curr_buf->m_file_pos;

        if (size > limit - total) {
            size = limit - total;
            
            off_t aligned = (curr_buf->m_file_pos + size + g_pagesize - 1) 
                & ~((off_t)g_pagesize - 1);
            
            if (aligned < curr_buf->m_file_last) {
                size = aligned - curr_buf->m_file_pos;
            }

            total += size;
            break;
        }

        total += size;
        prev_last = curr_buf->m_file_last;
        curr_chain = curr_chain->m_next;

    } while(curr_chain 
            && curr_chain->m_buffer->m_in_file
            && total < limit
            && m_buffer->fd() == curr_chain->m_buffer->fd());
    
    return curr_chain;
}

BufferChain *BufferChain::update_sent(size_t sent)
{
    BufferChain *in = this;
    
    for ( ; in; in = in->m_next) {

        if (in->m_buffer->special()) {
            continue;
        }

        if (sent == 0) {
            break;
        }

        off_t size = in->m_buffer->size();

        if (sent >= size) {
            sent -= size;

            if (in->m_buffer->in_memory()) {
                in->m_buffer->m_pos = in->m_buffer->m_last;
            }

            if (in->m_buffer->m_in_file) {
                in->m_buffer->m_file_pos = in->m_buffer->m_file_last;
            }

            continue;
        }

        /* else if (sent < size) */
        if (in->m_buffer->in_memory()) {
            in->m_buffer->m_pos += sent;
        }

        if (in->m_buffer->m_in_file) {
            in->m_buffer->m_file_pos += sent;
        }
        break;
    }

    return in;
}