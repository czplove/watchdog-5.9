/* $Header: /cvsroot/watchdog/watchdog/src/file_stat.c,v 1.2 2006/07/31 09:39:23 meskes Exp $ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <errno.h>
#include <time.h>
#include <sys/stat.h>
#include "extern.h"
#include "watch_err.h"

#if USE_SYSLOG
#include <syslog.h>
#endif

int check_file_stat(struct list *file)	//-检查文件状态,产生相应的系统记录
{
    struct stat buf;

    /* in filemode stat file */
    if (stat(file->name, &buf) == -1) {	//- 通过文件名filename获取文件信息，并保存在buf所指的结构体stat中; 执行成功则返回0，失败返回-1，错误代码存于errno
	int err = errno;

#if USE_SYSLOG
	syslog(LOG_ERR, "cannot stat %s (errno = %d = '%m')", file->name, err);	//-记录至系统记录。
#else				/* USE_SYSLOG */
	perror(progname);
#endif				/* USE_SYSLOG */
	/* on error ENETDOWN|ENETUNREACH we react as if we're in ping mode */
	if (softboot || err == ENETDOWN || err == ENETUNREACH)
	    return (err);	//-特定的错误我们做出反应
    } else if (file->parameter.file.mtime != 0) {

#if USE_SYSLOG
	/* do verbose logging */
	if (verbose && logtick && ticker == 1)
	    syslog(LOG_INFO, "file %s was last changed at %s.", file->name, ctime(&buf.st_mtime));
#endif

	if (time(NULL) - buf.st_mtime > file->parameter.file.mtime) {
	    /* file wasn't changed often enough */
#if USE_SYSLOG
	    syslog(LOG_ERR, "file %s was not changed in %d seconds.", file->name, file->parameter.file.mtime);
#else				/* USE_SYSLOG */
	    fprintf(stderr, "file %s was not changed in %d seconds.", file->name, file->parameter.file.mtime);
#endif				/* USE_SYSLOG */
	    return (ENOCHANGE);
	}
    }
    return (ENOERR);
}
