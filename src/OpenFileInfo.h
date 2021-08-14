#ifndef _OPEN_FILE_INFO_H_INCLUDED_
#define _OPEN_FILE_INFO_H_INCLUDED_

#include "Config.h"

class StringSlice;
class CachedOpenFile;
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

    Int open_and_stat_file(const StringSlice &name);
    Int stat_file(int fd);
    Int stat_file(const StringSlice &name);
    
#define O(member, default) unsigned m_##member:1;
    OpenFileInfo_BIT_MEMBER
#undef O
    int                             m_fd;
    ino_t                           m_ino;
    time_t                          m_mtime;
    off_t                           m_size;
    off_t                           m_fs_size;
    int                             m_err;
    const char *                    m_failed;

    off_t calculate_fs_size(struct stat &stat);
};


#endif