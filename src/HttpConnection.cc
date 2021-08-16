#include "HttpConnection.h"

HttpConnection::HttpConnection()
:   m_busy_buffers_num(0),
    m_busy_buffers(nullptr),
    m_free_buffers(nullptr)
{

}

HttpConnection::~HttpConnection()
{
    
}
