/* 
 * File:   wbt_os_util.h
 * Author: Fcten
 *
 * Created on 2016年4月07日, 下午12:58
 */

#ifndef __WBT_OS_UTIL_H__
#define	__WBT_OS_UTIL_H__

#ifdef	__cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdint.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <windows.h>

#define wbt_inline __inline

typedef DWORD  wbt_err_t;
typedef SOCKET wbt_socket_t;
typedef HANDLE wbt_fd_t;

#define ssize_t long
#define snprintf _snprintf
    
#define WBT_EPERM                  ERROR_ACCESS_DENIED
#define WBT_ENOENT                 ERROR_FILE_NOT_FOUND
#define WBT_ENOPATH                ERROR_PATH_NOT_FOUND
#define WBT_ENOMEM                 ERROR_NOT_ENOUGH_MEMORY
#define WBT_EACCES                 ERROR_ACCESS_DENIED
#define WBT_EEXIST                 ERROR_ALREADY_EXISTS
#define WBT_EEXIST_FILE            ERROR_FILE_EXISTS
#define WBT_EXDEV                  ERROR_NOT_SAME_DEVICE
#define WBT_ENOTDIR                ERROR_PATH_NOT_FOUND
#define WBT_EISDIR                 ERROR_CANNOT_MAKE
#define WBT_ENOSPC                 ERROR_DISK_FULL
#define WBT_EPIPE                  EPIPE
#define WBT_EAGAIN                 WSAEWOULDBLOCK
#define WBT_EINPROGRESS            WSAEINPROGRESS
#define WBT_ENOPROTOOPT            WSAENOPROTOOPT
#define WBT_EOPNOTSUPP             WSAEOPNOTSUPP
#define WBT_EADDRINUSE             WSAEADDRINUSE
#define WBT_ECONNABORTED           WSAECONNABORTED
#define WBT_ECONNRESET             WSAECONNRESET
#define WBT_ENOTCONN               WSAENOTCONN
#define WBT_ETIMEDOUT              WSAETIMEDOUT
#define WBT_ECONNREFUSED           WSAECONNREFUSED
#define WBT_ENAMETOOLONG           ERROR_BAD_PATHNAME
#define WBT_ENETDOWN               WSAENETDOWN
#define WBT_ENETUNREACH            WSAENETUNREACH
#define WBT_EHOSTDOWN              WSAEHOSTDOWN
#define WBT_EHOSTUNREACH           WSAEHOSTUNREACH
#define WBT_ENOMOREFILES           ERROR_NO_MORE_FILES
#define WBT_EILSEQ                 ERROR_NO_UNICODE_TRANSLATION
#define WBT_ELOOP                  0
#define WBT_EBADF                  WSAEBADF
#define WBT_EALREADY               WSAEALREADY
#define WBT_EINVAL                 WSAEINVAL
#define WBT_EMFILE                 WSAEMFILE
#define WBT_ENFILE                 WSAEMFILE


#define wbt_errno                  GetLastError()
#define wbt_socket_errno           WSAGetLastError()
#define wbt_set_errno(err)         SetLastError(err)
#define wbt_set_socket_errno(err)  WSASetLastError(err)

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
time_t wbt_get_file_last_write_time(wbt_fd_t fd);

int wbt_close_file(wbt_fd_t fd);

int wbt_truncate_file(wbt_fd_t fd, off_t length);
int wbt_sync_file(wbt_fd_t fd);
int wbt_sync_file_data(wbt_fd_t fd);

void wbt_msleep(int ms);

int gettimeofday(struct timeval *tp, void *tzp);

int wbt_getpid();

#ifdef	__cplusplus
}
#endif

#endif	/* __WBT_OS_UTIL_H__ */

