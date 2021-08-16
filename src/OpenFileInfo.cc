#include "OpenFileInfo.h"

#include "CachedOpenFile.h"


OpenFileInfo::OpenFileInfo()
:   
#define O(member, default) m_##member(default),
    OpenFileInfo_BIT_MEMBER
#undef O
    m_fd(-1),
    m_ino(0),
    m_mtime(0),
    m_size(0),
    m_fs_size(0),
    m_err(0),
    m_failed(nullptr)
{ }

OpenFileInfo::OpenFileInfo(const OpenFileInfo &right)
:   
#define O(member, default) m_##member(right.m_##member),
    OpenFileInfo_BIT_MEMBER
#undef O
    m_fd(right.m_fd),
    m_ino(right.m_ino),
    m_mtime(right.m_mtime),
    m_size(right.m_size),
    m_fs_size(right.m_fs_size),
    m_err(right.m_err),
    m_failed(right.m_failed)
{ }


Int OpenFileInfo::stat_file(int fd) {

    struct stat stat;

    if (::fstat(fd, &stat) == -1) {
        m_err = errno;
        m_failed = "::fstat()";
        m_fd = -1;
        return ERROR;
    }
    
    m_fd = fd;
    m_ino = stat.st_ino;
    m_mtime = stat.st_mtime;
    m_size = stat.st_size;
    m_fs_size = calculate_fs_size(stat);

    m_is_dir = S_ISDIR(stat.st_mode);
    m_is_file = S_ISREG(stat.st_mode);
    m_is_link = S_ISLNK(stat.st_mode);
    m_is_exec = (stat.st_mode & S_IEXEC) == S_IEXEC;

    if (m_is_dir) {
        
        if(::close(fd) == -1) {
            int err = errno;
            LOG_ERROR(LogLevel::alert, "OpenFileInfo::stat_file()/::close() 失败，"
                        "errno %d: %s\n ==== %s %d\n", 
                        err, strerror(err), __FILE__, __LINE__);
        }
        m_fd = -1; 

    } else {
        if (MIN_READ_AHEAD_SIZE < m_size) {
            int err = posix_fadvise(fd, 0, 0, POSIX_FADV_SEQUENTIAL);

            if (err != 0) {
                errno = err;
                LOG_ERROR(LogLevel::alert, 
                    "::open_and_stat_file()/::posix_fadvise() 失败，"
                    "errno %d: %s\n ==== %s %d\n", 
                    err, strerror(err), __FILE__, __LINE__);
            }

            if (m_size >= MIN_DIRECTIO_SIZE) {
                int flags = fcntl(fd, F_GETFL);

                if (flags != -1) {
                    if (fcntl(fd, F_SETFL, flags|O_DIRECT) != -1) {
                        m_is_directio = true;
                    }
                }
                if (!m_is_directio) {
                    int err = errno;
                    LOG_ERROR(LogLevel::alert, 
                        "::open_and_stat_file()/设置直接io失败，"
                        "errno %d: %s\n ==== %s %d\n", 
                        err, strerror(err), __FILE__, __LINE__);
                }
            }
        }
    }
    return OK;
}

Int OpenFileInfo::stat_file(const StringSlice &name) {

    struct stat stat;

    if (::stat((char*)name.m_data, &stat) == -1) {
        m_err = errno;
        m_failed = "::stat()";
        m_fd = -1;
        return ERROR;
    }

    m_ino = stat.st_ino;
    m_mtime = stat.st_mtime;
    m_size = stat.st_size;
    m_fs_size = calculate_fs_size(stat);

    m_is_dir = S_ISDIR(stat.st_mode);
    m_is_file = S_ISREG(stat.st_mode);
    m_is_link = S_ISLNK(stat.st_mode);
    m_is_exec = (stat.st_mode & S_IEXEC) == S_IEXEC;
    return OK;
}
off_t OpenFileInfo::calculate_fs_size(struct stat &stat) {
    
    off_t block_size = stat.st_blocks * 512;

    return (block_size < stat.st_size + 8 * stat.st_blksize &&
            block_size > stat.st_size) ? block_size : stat.st_size;
};

Int OpenFileInfo::open_and_stat_file(const StringSlice &name)
{
    int fd;
    if (!m_log) {
        fd = ::open((char*)name.m_data, O_RDONLY|O_NONBLOCK, 0); 

    } else {
        fd = ::open((char*)name.m_data, O_CREAT|O_APPEND|O_WRONLY, 0644);
    }

    if (fd == -1) {
        m_err = errno;
        m_failed = "::open()";
        m_fd = -1;
        return ERROR;
    }

    if (stat_file(fd) == ERROR) {
        
        if(::close(fd) == -1) {
            int err = errno;
            LOG_ERROR(LogLevel::alert, "::open_and_stat_file()/::close() 失败，"
                        "errno %d: %s\n ==== %s %d\n", 
                        err, strerror(err), __FILE__, __LINE__);
        }
        m_fd = -1; 
        return ERROR;
    }
    return OK;
}
