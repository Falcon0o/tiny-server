#ifndef _FILE_H_INCLUDED_
#define _FILE_H_INCLUDED_

#include "Config.h"
#include "StringSlice.h" 


class OpenFileInfo 
{
    #define OpenFileInfo_BIT_MEMBER                 \
    O(test_dir,             false)              \
    O(log,                  false)              \
    O(read_ahead,           false)              \
    O(is_directio,          false)              \
    O(is_dir,               false)              \
    O(is_file,              false)              \
    O(is_link,              false)              \
    O(is_exec,              false)              

public:
    OpenFileInfo();
    OpenFileInfo(const OpenFileInfo&);
    ~OpenFileInfo() = default;

#define O(member, default) unsigned m_##member:1;
    OpenFileInfo_BIT_MEMBER
#undef O
    int                             m_fd;
    ino_t                           m_ino;
    time_t                          m_mtime;
    off_t                           m_size;
    off_t                           m_fs_size;
};

Int open_and_stat_file(const char *path, OpenFileInfo&);


class File {
public:

    int                 m_fd;
    StringSlice         m_name;
    bool                m_directio;
    // OpenFileInfo        m_openfileinfo;
};
#endif