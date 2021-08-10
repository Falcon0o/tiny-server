#include "File.h"

OpenFileInfo::OpenFileInfo()
:   
#define O(member, default) m_##member(default),
    OpenFileInfo_BIT_MEMBER
#undef O
    m_fd(-1),
    m_ino(0),
    m_mtime(0),
    m_size(0),
    m_fs_size(0)
{

}

OpenFileInfo::OpenFileInfo(const OpenFileInfo &right)
:   
#define O(member, default) m_##member(right.m_##member),
    OpenFileInfo_BIT_MEMBER
#undef O
    m_fd(right.m_fd),
    m_ino(right.m_ino),
    m_mtime(right.m_mtime),
    m_size(right.m_size),
    m_fs_size(right.m_fs_size)
{
}

Int open_and_stat_file(const char *path, OpenFileInfo &ofi)
{
    if (ofi.m_fd != -1) {
        assert(0);

    } else if (ofi.m_test_dir){
        assert(0);
    }

    int fd; 
    if (!ofi.m_log) {
        fd = open(path, O_RDONLY|O_NONBLOCK, 0); 
    } else {
        fd = open(path, O_CREAT|O_APPEND|O_WRONLY, 0644);
    }

    if (fd == -1) {
        ofi.m_fd = -1;
        return ERROR;
    }

    struct stat stat;

    if (fstat(fd, &stat) == -1) {
        log_error(LogLevel::crit, "(%s: %d)", __FILE__, __LINE__);
        
        if (close(fd) == -1) {
            log_error(LogLevel::alert, "(%s: %d)", __FILE__, __LINE__);
        }

        ofi.m_fd = -1;
        return ERROR;
    }

    if (S_ISDIR(stat.st_mode)) {
        if (close(fd) == -1) {
            log_error(LogLevel::alert, "(%s: %d)", __FILE__, __LINE__);
        }
        ofi.m_fd = -1;

    } else {
        ofi.m_fd = fd;

        if (stat.st_size > MIN_READ_AHEAD_SIZE) {
            int err = posix_fadvise(fd, 0, 0, POSIX_FADV_SEQUENTIAL);

            if (err != 0) {
                errno = err;
                log_error(LogLevel::alert, "(%s: %d) ===> posix_fadvise(POSIX_FADV_SEQUENTIAL)" 
                                            "failed\n", __FILE__, __LINE__);
            }
        }

        if (stat.st_size >= MIN_DIRECTIO_SIZE) {
            int flags = fcntl(fd, F_GETFL);

            if (flags != -1) {
                if (fcntl(fd, F_SETFL, flags|O_DIRECT) != -1) {
                    ofi.m_is_directio = true;
                }
            }
            if (!ofi.m_is_directio) {
                log_error(LogLevel::alert, "(%s: %d) ===> 设置直接io失败 \n",
                                 __FILE__, __LINE__);
            }
        }
    }

done:

    ofi.m_ino = stat.st_ino;
    ofi.m_mtime = stat.st_mtime;
    ofi.m_size = stat.st_size;

    /* 如果 */
    auto get_fs_size = [&stat]()->off_t {
        off_t block_size = stat.st_blocks * 512;
        return (block_size < stat.st_size + 8 * stat.st_blksize &&
                block_size > stat.st_size) ? block_size : stat.st_size;
    };
    ofi.m_fs_size = get_fs_size();
    
    ofi.m_is_dir = S_ISDIR(stat.st_mode);
    ofi.m_is_file = S_ISREG(stat.st_mode);
    ofi.m_is_link = S_ISLNK(stat.st_mode);
    ofi.m_is_exec = (stat.st_mode & S_IEXEC) == S_IEXEC;
    return OK;
}
