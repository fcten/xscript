/* 
 * File:   wbt_os_util.h
 * Author: Fcten
 *
 * Created on 2015年4月29日, 下午4:35
 */

#ifndef __WBT_OS_UTIL_H__
#define	__WBT_OS_UTIL_H__

#ifdef	__cplusplus
extern "C" {
#endif

#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <grp.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
  
#define wbt_inline inline

typedef int wbt_err_t;
typedef int wbt_socket_t;
typedef int wbt_fd_t;

#define WBT_EPERM         EPERM
#define WBT_ENOENT        ENOENT
#define WBT_ENOPATH       ENOENT
#define WBT_ESRCH         ESRCH
#define WBT_EINTR         EINTR
#define WBT_ECHILD        ECHILD
#define WBT_ENOMEM        ENOMEM
#define WBT_EACCES        EACCES
#define WBT_EBUSY         EBUSY
#define WBT_EEXIST        EEXIST
#define WBT_EEXIST_FILE   EEXIST
#define WBT_EXDEV         EXDEV
#define WBT_ENOTDIR       ENOTDIR
#define WBT_EISDIR        EISDIR
#define WBT_EINVAL        EINVAL
#define WBT_ENFILE        ENFILE
#define WBT_EMFILE        EMFILE
#define WBT_ENOSPC        ENOSPC
#define WBT_EPIPE         EPIPE
#define WBT_EINPROGRESS   EINPROGRESS
#define WBT_ENOPROTOOPT   ENOPROTOOPT
#define WBT_EOPNOTSUPP    EOPNOTSUPP
#define WBT_EADDRINUSE    EADDRINUSE
#define WBT_ECONNABORTED  ECONNABORTED
#define WBT_ECONNRESET    ECONNRESET
#define WBT_ENOTCONN      ENOTCONN
#define WBT_ETIMEDOUT     ETIMEDOUT
#define WBT_ECONNREFUSED  ECONNREFUSED
#define WBT_ENAMETOOLONG  ENAMETOOLONG
#define WBT_ENETDOWN      ENETDOWN
#define WBT_ENETUNREACH   ENETUNREACH
#define WBT_EHOSTDOWN     EHOSTDOWN
#define WBT_EHOSTUNREACH  EHOSTUNREACH
#define WBT_ENOSYS        ENOSYS
#define WBT_ECANCELED     ECANCELED
#define WBT_EILSEQ        EILSEQ
#define WBT_ENOMOREFILES  0
#define WBT_ELOOP         ELOOP
#define WBT_EBADF         EBADF

#if (WBT_HAVE_OPENAT)
#define WBT_EMLINK        EMLINK
#endif

#if (__hpux__)
#define WBT_EAGAIN        EWOULDBLOCK
#else
#define WBT_EAGAIN        EAGAIN
#endif


#define wbt_errno                  errno
#define wbt_socket_errno           errno
#define wbt_set_errno(err)         errno = err
#define wbt_set_socket_errno(err)  errno = err

int wbt_get_file_path_by_fd(int fd, void * buf, size_t buf_len);
int wbt_getopt(int argc,char * const argv[ ],const char * optstring);

int wbt_nonblocking(wbt_socket_t s);
int wbt_blocking(wbt_socket_t s);

int wbt_close_socket(wbt_socket_t s);

int wbt_trylock_fd(wbt_fd_t fd);
int wbt_lock_fd(wbt_fd_t fd);
int wbt_unlock_fd(wbt_fd_t fd);
wbt_fd_t wbt_lock_create(const char *name);

wbt_fd_t wbt_open_logfile(const char *name);
wbt_fd_t wbt_open_datafile(const char *name);
wbt_fd_t wbt_open_tmpfile(const char *name);
wbt_fd_t wbt_open_file(const char *name);

int wbt_delete_file(const char *name);

ssize_t wbt_read_file(wbt_fd_t fd, void *buf, size_t count, off_t offset);
ssize_t wbt_write_file(wbt_fd_t fd, const void *buf, size_t count, off_t offset);
ssize_t wbt_append_file(wbt_fd_t fd, const void *buf, size_t count);

ssize_t wbt_get_file_size(wbt_fd_t fd);

int wbt_close_file(wbt_fd_t fd);

int wbt_truncate_file(wbt_fd_t fd, off_t length);
int wbt_sync_file(wbt_fd_t fd);
int wbt_sync_file_data(wbt_fd_t fd);

void wbt_msleep(int ms);

int wbt_getpid();

#ifdef	__cplusplus
}
#endif

#endif	/* __WBT_OS_UTIL_H__ */

