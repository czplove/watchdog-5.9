﻿/* $Header: /cvsroot/watchdog/watchdog/src/file_table.c,v 1.2 2006/07/31 09:39:23 meskes Exp $ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/syslog.h>

#include "extern.h"
#include "watch_err.h"

int check_file_table(void)	//-检查了一个特定的系统文件,然后做出相应的判断,这个可以保证系统可靠运行,但是特定系统要校验下文件是否存在
{
    int fd;

    /* open a file */
    fd = open("/proc/uptime", O_RDONLY);
    if (fd == -1) {
	int err = errno;

	if (err == ENFILE) {
	    /* we need a reboot if ENFILE is returned (file table overflow) */
#if USE_SYSLOG
	    syslog(LOG_ERR, "file table overflow detected!\n");
#endif				/* USE_SYSLOG */
	    return (ENFILE);	//-复位之前记录了系统日志
	} else {
#if USE_SYSLOG
	    errno = err;
	    syslog(LOG_ERR, "cannot open /proc/uptime (errno = %d = '%m')", err);
#else				/* USE_SYSLOG */
	    perror(progname);
#endif				/* USE_SYSLOG */

	    if (softboot)
		return (err);
	}
    } else {
	if (close(fd) < 0) {
	    int err = errno;

#if USE_SYSLOG
	    syslog(LOG_ERR, "close /proc/uptime gave errno = %d = '%m'", err);
#else				/* USE_SYSLOG */
	    perror(progname);
#endif				/* USE_SYSLOG */
	    if (softboot)
		return (err);
	}
    }

    return (ENOERR);
}
