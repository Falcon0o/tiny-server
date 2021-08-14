#ifndef _FILE_H_INCLUDED_
#define _FILE_H_INCLUDED_

#include "Config.h"
#include "StringSlice.h" 


class File {
public:

    int                 m_fd;
    StringSlice         m_name;
    bool                m_directio;
    // OpenFileInfo        m_openfileinfo;
};
#endif