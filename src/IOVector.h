#ifndef _IO_VECTOR_H_INCLUDED_
#define _IO_VECTOR_H_INCLUDED_

#include "Config.h"

class BufferChain;
class IOVector {
public:
    IOVector(struct iovec *iov, size_t cap) 
    :   m_array(iov), m_capacity(cap), m_len_sum(0), m_size(0) {}
    ~IOVector() {}

    /* 返回值为从此处开始的 chain 未加入 io vector 中
     */
    BufferChain *fill_with_output_chain(BufferChain &in, size_t limit);
    size_t capacity() const { return m_capacity; }
    size_t size() const { return m_size; }
    iovec &operator[](size_t i);
    const iovec *data() { return m_array; }
    size_t total_length() const { return m_len_sum; } 
    iovec *emplace_back(void *base, size_t len);
private:
    iovec *const            m_array;
    size_t                  m_len_sum;
    size_t                  m_size;
    const size_t            m_capacity;
};

#endif