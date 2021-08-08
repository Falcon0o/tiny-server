#include "BufferChain.h"

#include <sys/uio.h>

#include "Buffer.h"
#include "Cycle.h"


// BufferChain *BufferChain::coalesce_file(off_t limit, off_t &total)
// {
//     int pagesize = Cycle::singleton()->getpagesize();
    
//     total = 0;

//     off_t prev_last;
//     BufferChain *curr_chain = this;
//     do {
//         Buffer *curr_buf = curr_chain->buffer();

//         off_t size = curr_buf->file_size();

//         if (size > limit - total) {
//             size = limit - total;
            
//             off_t aligned = (curr_buf->file_pos() + size + pagesize - 1) 
//                 & ~((off_t)pagesize - 1);
            
//             if (aligned < curr_buf->file_last()) {
//                 size = aligned - curr_buf->file_pos();
//             }

//             total += size;
//             break;
//         }

//         total += size;
//         prev_last = curr_buf->file_last();
//         curr_chain = curr_chain->next();

//     } while(curr_chain 
//             && curr_chain->buffer()->in_file()
//             && total < limit
//             && m_buffer->get_fd() == curr_chain->buffer()->get_fd());
    
//     return curr_chain;
// }

// BufferChain *BufferChain::update_sent(size_t sent)
// {
//     BufferChain *in = this;
    
//     for ( ; in; in = in->next()) {

//         if (in->buffer()->special()) {
//             continue;
//         }

//         if (sent == 0) {
//             break;
//         }

//         off_t size = in->buffer()->size();

//         if (sent >= size) {
//             sent -= size;

//             if (in->buffer()->in_memory()) {
//                 in->buffer()->set_pos(in->buffer()->last());
//             }

//             if (in->buffer()->in_file()) {
//                 in->buffer()->set_file_pos(in->buffer()->file_last());
//             }

//             continue;
//         }

//         /* else if (sent < size) */
//         if (in->buffer()->in_memory()) {
//             in->buffer()->set_pos(in->buffer()->pos() + sent);
//         }

//         if (in->buffer()->in_file()) {
//             in->buffer()->set_file_pos(in->buffer()->file_pos() + sent);
//         }
//         break;
//     }

//     return in;
// }