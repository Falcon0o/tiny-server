#ifndef _CACHED_OPEN_FILE_H_INCLUDED_
#define _CACHED_OPEN_FILE_H_INCLUDED_

#include "Config.h"


#include "StringSlice.h"

class OpenFileInfo;

class CachedOpenFile {

public:
    using ExpireQueue   = std::list<CachedOpenFile*>;

    class CompareStringSlice {
    public:
        bool operator()(
            const std::pair<uint32_t, StringSlice> &left, 
            const std::pair<uint32_t, StringSlice> &right);
    };

    using FileRBTree    = std::map<std::pair<uint32_t, StringSlice>, CachedOpenFile*, 
                                    class CompareStringSlice>;
                                    
    static Int open_cached_file(const StringSlice &name, OpenFileInfo &info);
    
private:
    CachedOpenFile() = default;
    ~CachedOpenFile() = default; 

    void update_by_info(const OpenFileInfo &info);
    void propagate_info(OpenFileInfo &info) const;

    static void expire_old_cached_files(bool force);
    static Int  construct_cached_file(const StringSlice &name, OpenFileInfo &info);
    static void destruct_cached_file(CachedOpenFile *file);
    int                             m_fd;
    ino_t                           m_ino;
    time_t                          m_mtime;
    off_t                           m_size;

    uint32_t                        m_uses;
    int                             m_err;

    // unsigned                        m_count:24;     /* 只对文件有效 */
    // unsigned                        m_close:1;
    unsigned                        m_is_dir:1;
    unsigned                        m_is_directio:1;
    unsigned                        m_is_file:1;
    unsigned                        m_is_link:1;
    unsigned                        m_is_exec:1;

    time_t                          m_created;
    time_t                          m_accessed;

    StringSlice                     m_name;
    uint32_t                        m_hash;

    static ExpireQueue              s_expire_queue;
    ExpireQueue::iterator           m_queue_iter;
    
    static FileRBTree               s_file_rbtree;
    FileRBTree::iterator            m_rbtree_iter;

    static uInt                     s_max;
    static time_t                   s_inactive;
};

#endif

