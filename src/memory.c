/* $Header: /cvsroot/watchdog/watchdog/src/memory.c,v 1.2 2006/07/31 09:39:23 meskes Exp $ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include "extern.h"
#include "watch_err.h"

#if USE_SYSLOG
#include <syslog.h>
#endif

#define FREEMEM		"MemFree:"
#define FREESWAP	"SwapFree:"

int check_memory(void)	//-机器的内存使用信息,对信息进行检测并记录到系统日志
{
    char buf[1024], *ptr1, *ptr2;
    unsigned int free;

    /* is the memory file open? */
    if (mem == -1)
	return (ENOERR);

    /* position pointer at start of file */
    if (lseek(mem, 0, SEEK_SET) < 0) {	//-定位到文件开始,应该是为了可靠处理的
	int err = errno;

#if USE_SYSLOG
	syslog(LOG_ERR, "lseek /proc/meminfo gave errno = %d = '%m'", err);
#else				/* USE_SYSLOG */
	perror(progname);
#endif				/* USE_SYSLOG */
	if (softboot)
	    return (err);

	return (ENOERR);
    }
    
    /* read the file */
    if (read(mem, buf, sizeof(buf)) < 0) {
	int err = errno;

#if USE_SYSLOG
	syslog(LOG_ERR, "read /proc/meminfo gave errno = %d = '%m'", err);
#else				/* USE_SYSLOG */
	perror(progname);
#endif				/* USE_SYSLOG */
	if (softboot)
	    return (err);

	return (ENOERR);
    }
    
    ptr1 = strstr(buf, FREEMEM);	//-LowFree与HighFree的总和，被系统留着未使用的内存
    ptr2 = strstr(buf, FREESWAP);	//-未被使用交换空间的大小
    
    if (!ptr1 || !ptr2) {
#if USE_SYSLOG
	syslog(LOG_ERR, "/proc/meminfo contains invalid data (read = %s)", buf);
#else				/* USE_SYSLOG */
	perror(progname);
#endif				/* USE_SYSLOG */
	if (softboot)
	    return (EINVMEM);

	return (ENOERR);
    }

    /* we only care about integer values */
    free = atoi(ptr1+strlen(FREEMEM)) + atoi(ptr2+strlen(FREESWAP));	//-计算出系统的总值

#if USE_SYSLOG
    if (verbose && logtick && ticker == 1)
	syslog(LOG_INFO, "currently there are %d kB of free memory available", free);
#endif				/* USE_SYSLOG */

    if (free < minpages * (EXEC_PAGESIZE / 1024)) {
#if USE_SYSLOG
	syslog(LOG_ERR, "memory %d kB is less than %d pages", free, minpages);
#endif				/* USE_SYSLOG */
	return (ENOMEM);
    }
    
    return (ENOERR);
}
