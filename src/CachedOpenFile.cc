#include "CachedOpenFile.h"


#include "OpenFileInfo.h"
#include "Timer.h"

CachedOpenFile::ExpireQueue CachedOpenFile::s_expire_queue;
CachedOpenFile::FileRBTree  CachedOpenFile::s_file_rbtree;

uInt    CachedOpenFile::s_max       = OPEN_FILE_CACHE_MAX;
time_t  CachedOpenFile::s_inactive  = OPEN_FILE_CACHE_INACTIVE;

Int CachedOpenFile::
construct_cached_file(const StringSlice &name, OpenFileInfo &info)
{
    Int rc = info.open_and_stat_file(name);

    if (rc == ERROR) {
        return ERROR;   
    }

    CachedOpenFile *file = (CachedOpenFile*)malloc(sizeof(CachedOpenFile));
    
    if (!file) {
        return ERROR;
    }

    file->m_name.m_data = (u_char*)::malloc(name.m_len + 1);
    if (file->m_name.m_data == nullptr) {
        ::free(file);
        return ERROR;
    }

    file->m_name.m_len = name.m_len;
    ::memcpy(file->m_name.m_data, name.m_data, name.m_len + 1);

    file->m_hash = name.crc32_long();
    auto ans = s_file_rbtree.insert({{file->m_hash, file->m_name}, file});

    if (!ans.second) {
        if (info.m_fd != -1) {
            if (::close(info.m_fd) == -1) {
                debug_point();
            }
        }
        ::free(file->m_name.m_data);
        ::free(file);
        return ERROR;
    }

    file->m_rbtree_iter = ans.first;

    file->m_fd = info.m_fd;
    file->m_uses = 1;
    file->update_by_info(info);
    file->m_created = g_timer->cached_time_sec();
    
    file->m_accessed = file->m_created;
    s_expire_queue.emplace_front(file);
    file->m_queue_iter = s_expire_queue.begin();

    return file->m_err == 0 ? OK : ERROR;
}

void CachedOpenFile::update_by_info(const OpenFileInfo &info)
{
    m_err = info.m_err;

    if (m_err == 0) {
        m_ino = info.m_ino;
        m_mtime = info.m_mtime;
        m_size = info.m_size;

        m_is_dir = info.m_is_dir;
        m_is_directio = info.m_is_directio;
        m_is_file = info.m_is_file;
        m_is_link = info.m_is_link;
        m_is_exec = info.m_is_exec;
    }
}

void CachedOpenFile::expire_old_cached_files(bool force)
{
    time_t curr_sec = g_timer->cached_time_sec();

    int n = force ? 0 : 1; 
    while(n < 3) {

        if (s_expire_queue.empty()) {
            return;
        }

        ExpireQueue::iterator iter = s_expire_queue.rbegin().base();
        CachedOpenFile *file = *iter;

        if (n++ != 0 && curr_sec - file->m_accessed <= s_inactive) {
            return;
        }

        s_expire_queue.erase(iter);
        s_file_rbtree.erase(file->m_rbtree_iter);

        destruct_cached_file(file);
    }
}

void CachedOpenFile::destruct_cached_file(CachedOpenFile *file)
{
    if (file->m_fd != -1) {
        if (::close(file->m_fd) == -1) {
            debug_point();
        }
    }

    ::free(file->m_name.m_data);
    ::free(file);
}

Int CachedOpenFile::open_cached_file(const StringSlice &name, OpenFileInfo &info)
{
    time_t curr_sec = g_timer->cached_time_sec();

    u_int32_t name_crc32 = name.crc32_long();
    FileRBTree::iterator iter = s_file_rbtree.find({name_crc32, name});

    if (iter == s_file_rbtree.end()) {
        if (s_file_rbtree.size() >= s_max) {
            expire_old_cached_files(true);
        }
        
        return construct_cached_file(name, info);
    }

    /* 在红黑树中查找到文件缓存 */
    CachedOpenFile *file = iter->second;
    file->m_uses++;

    if (file->m_queue_iter != s_expire_queue.end()) {
        s_expire_queue.erase(file->m_queue_iter);
        file->m_queue_iter = s_expire_queue.end();
    }

    if (curr_sec - file->m_created < OPEN_FILE_CACHE_VALID) {
        /* 以有效时间为周期更新文件信息，低于这个时间不更新缓存文件信息 */
        file->propagate_info(info);

        file->m_accessed = curr_sec;
        s_expire_queue.emplace_front(file);
        file->m_queue_iter = s_expire_queue.begin();

        return file->m_err == 0 ? OK : ERROR;
    }

    /* 以有效时间为周期更新文件信息，需要更新缓存文件信息 */
    /* 获取文件描述符以外的信息 */
    int rc = info.stat_file(name);
    if (rc == ERROR) {
        s_file_rbtree.erase(iter);
        destruct_cached_file(file);
        return ERROR;
    }

    if (file->m_is_dir && info.m_is_dir) {
        file->m_fd = -1;
        file->update_by_info(info);
        file->m_created = curr_sec;

        file->m_accessed = curr_sec;
        s_expire_queue.emplace_front(file);
        file->m_queue_iter = s_expire_queue.begin();
        return file->m_err == 0 ? OK : ERROR;
    }


    if (file->m_ino != info.m_ino) {
        if (file->m_fd) {
            if (::close(file->m_fd) == -1) {
                debug_point();
            }
            file->m_fd = -1;
        }
    }

    if (info.open_and_stat_file(name) == ERROR) {
        return ERROR;   
    }

    file->m_fd = info.m_fd;
    file->update_by_info(info);
    file->m_created = g_timer->cached_time_sec();
    
    file->m_accessed = file->m_created;
    s_expire_queue.emplace_front(file);
    file->m_queue_iter = s_expire_queue.begin();

    return file->m_err == 0 ? OK : ERROR;
}


void CachedOpenFile::propagate_info(OpenFileInfo &info) const {

    info.m_err = m_err;
    if (m_err == 0) {
        info.m_fd = m_fd;
        info.m_ino = m_ino;
        info.m_mtime = m_mtime;
        info.m_size = m_size;
        
        info.m_is_dir = m_is_dir;
        info.m_is_directio = m_is_directio;
        info.m_is_file = m_is_file;
        info.m_is_link = m_is_link;
        info.m_is_exec = m_is_exec;

    } else {
        info.m_failed = "::open()";
    }    
}

bool CachedOpenFile::
CompareStringSlice::operator()(const std::pair<uint32_t, StringSlice> &left, 
                               const std::pair<uint32_t, StringSlice> &right)
{
    if (left.first != right.first) {
        return left.first < right.first;
    }

    if (left.second.m_len != left.second.m_len) {
        return left.second.m_len < left.second.m_len;
    }

    for (size_t i = 0; i < left.second.m_len; ++i) {

        if (left.second.m_data[i] == right.second.m_data[i]) {
            continue;
        }

        return left.second.m_data[i] < right.second.m_data[i];
    }

    return false;
}
