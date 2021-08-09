#ifndef _FILE_H_INCLUDED_
#define _FILE_H_INCLUDED_

#include "Config.h"
#include "StringSlice.h" 
class OpenFileInfo;
class File {
public:
    File(const OpenFileInfo &fi, const StringSlice &name);
    ~File();

    int                 m_fd;
private:
    StringSlice         m_name;
    bool                m_directio;
    // OpenFileInfo        m_openfileinfo;
};
#endif