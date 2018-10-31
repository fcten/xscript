/* 
 * File:   wbt_os_util.c
 * Author: Fcten
 *
 * Created on 2015年4月29日, 下午4:35
 */

#include "../../webit.h"
#include "../../common/wbt_memory.h"
#include "wbt_os_util.h"

int wbt_nonblocking(wbt_socket_t s) {
    unsigned long nb = 1;
    return ioctlsocket(s, FIONBIO, &nb);
}

int wbt_blocking(wbt_socket_t s) {
    unsigned long nb = 0;
    return ioctlsocket(s, FIONBIO, &nb);
}

/* 尝试对指定文件加锁
 * 如果加锁失败，进程将会睡眠，直到加锁成功
 */
int wbt_trylock_fd(wbt_fd_t fd) {
	OVERLAPPED oapped;
	if (LockFileEx(fd, LOCKFILE_EXCLUSIVE_LOCK, 0, 200, 0, &oapped)) {
		return 0;
	}

	return -1;
}

int wbt_close_socket(wbt_socket_t s) {
	return closesocket(s);
}

/* 尝试对指定文件加锁
 * 如果加锁失败，立刻出错返回
 */
int wbt_lock_fd(wbt_fd_t fd) {
	OVERLAPPED oapped;
	if (LockFileEx(fd, LOCKFILE_FAIL_IMMEDIATELY | LOCKFILE_EXCLUSIVE_LOCK, 0, 200, 0, &oapped)) {
		return 0;
	}

	return -1;
}

int wbt_unlock_fd(wbt_fd_t fd) {
	OVERLAPPED oapped;
	if (UnlockFileEx(fd, 0, 200, 0, &oapped)) {
		return 0;
	}

	return -1;
}

wbt_fd_t wbt_lock_create(const char *name) {
	return CreateFile(name, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, NULL);
}

wbt_fd_t wbt_open_logfile(const char *name) {
	return CreateFile(name, GENERIC_READ | FILE_APPEND_DATA, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
}

wbt_fd_t wbt_open_datafile(const char *name) {
	return CreateFile(name, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
}

wbt_fd_t wbt_open_tmpfile(const char *name) {
	wbt_fd_t fd = CreateFile(name, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, NULL);
	if ((int)fd > 0) {
		wbt_truncate_file(fd, 0);
	}
	return fd;
}

wbt_fd_t wbt_open_file(const char *name) {
	return CreateFile(name, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
}

int wbt_delete_file(const char *name) {
	if (DeleteFile(name)) {
		return 0;
	} else {
		return -1;
	}
}

ssize_t wbt_read_file(wbt_fd_t fd, void *buf, size_t count, off_t offset) {
	u_long nread;
	wbt_err_t err;
	OVERLAPPED oapped;

	oapped.Internal = 0;
	oapped.InternalHigh = 0;
	oapped.Offset = (u_long)offset;
	oapped.OffsetHigh = 0;
	oapped.hEvent = NULL;
	
	if (ReadFile(fd, buf, count, &nread, &oapped) == 0) {
		err = wbt_errno;

		if (err == ERROR_HANDLE_EOF) {
			return 0;
		}

		return -1;
	}

	return nread;
}

ssize_t wbt_write_file(wbt_fd_t fd, const void *buf, size_t count, off_t offset) {
	u_long nwrite;
	OVERLAPPED oapped;

	oapped.Internal = 0;
	oapped.InternalHigh = 0;
	oapped.Offset = (u_long)offset;
	oapped.OffsetHigh = 0;
	oapped.hEvent = NULL;

	if (WriteFile(fd, buf, count, &nwrite, &oapped) == 0) {
		return -1;
	}

	return nwrite;
}

ssize_t wbt_append_file(wbt_fd_t fd, const void *buf, size_t count) {
	u_long nwrite;
	if (WriteFile(fd, buf, count, &nwrite, NULL) == 0) {
		return -1;
	}

	return nwrite;
}

void wbt_msleep(int ms) {
	Sleep(ms);
}

int gettimeofday(struct timeval *tp, void *tzp) {
	time_t clock;
	struct tm stm;
	SYSTEMTIME wtm;
	GetLocalTime(&wtm);
	stm.tm_year = wtm.wYear - 1900;
	stm.tm_mon = wtm.wMonth - 1;
	stm.tm_mday = wtm.wDay;
	stm.tm_hour = wtm.wHour;
	stm.tm_min = wtm.wMinute;
	stm.tm_sec = wtm.wSecond;
	stm.tm_isdst = -1;
	clock = mktime(&stm);
	tp->tv_sec = (long)clock;
	tp->tv_usec = wtm.wMilliseconds * 1000;
	return (0);
}

ssize_t wbt_get_file_size(wbt_fd_t fd) {
	return GetFileSize(fd, NULL);
}

time_t wbt_get_file_last_write_time(wbt_fd_t fd) {
	FILETIME fCreateTime, fAccessTime, fWriteTime;
	
	if (GetFileTime(fd, &fCreateTime, &fAccessTime, &fWriteTime)) {
		return (time_t)fWriteTime.dwLowDateTime;
	} else {
		return 0;
	}
}

int wbt_close_file(wbt_fd_t fd) {
	if (CloseHandle(fd)) {
		return 0;
	} else {
		return -1;
	}
}

int wbt_truncate_file(wbt_fd_t fd, off_t length) {
	if (SetFilePointer(fd, length, 0, FILE_BEGIN) == HFILE_ERROR) {
		return -1;
	}

	if (SetEndOfFile(fd)) {
		return 0;
	} else {
		return -1;
	}
}

int wbt_sync_file(wbt_fd_t fd) {
	if (FlushFileBuffers(fd)) {
		return 0;
	} else {
		return -1;
	}
}

int wbt_sync_file_data(wbt_fd_t fd) {
	if (FlushFileBuffers(fd)) {
		return 0;
	} else {
		return -1;
	}
}

int wbt_getpid() {
	return GetCurrentProcessId();
}