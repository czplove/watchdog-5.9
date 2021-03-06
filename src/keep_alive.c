﻿/* $Header: /cvsroot/watchdog/watchdog/src/keep_alive.c,v 1.2 2006/07/31 09:39:23 meskes Exp $ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <errno.h>
#include <sys/syslog.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#include "extern.h"
#include "watch_err.h"

/* write a heartbeat file */
int write_heartbeat(void)
{
    time_t timenow;
    struct tm *tm;
    char tbuf[TS_SIZE + 1];
    char tbufw[TS_SIZE + 1];

    if (hb == NULL)
	    return (ENOERR);

    /* MJ 16/2/2001 keep a rolling buffer in a file of writes to the
       watchdog device, any gaps in this will indicate a reboot */

    timenow = time(NULL);
    if (timenow != -1) {
        tm = gmtime(&timenow);	//-获取当前时间和日期
        /* Get the seconds since seconds since 00:00:00, Jan 1, 1970 */
        strftime(tbuf, TS_SIZE - 1, "%s", tm);	//-将时间格式化
        /* Make it the right width */ 
        sprintf(tbufw, "%*s\n", TS_SIZE - 1, tbuf);
        /* copy it to the buffer */
        memcpy(timestamps + (lastts * TS_SIZE), tbufw, TS_SIZE);

        // success
        if (nrts < hbstamps) 
            nrts++;
        ++lastts;
        lastts = lastts % hbstamps;	//-保证不过界
        
        // write the buffer to the file
        rewind(hb);	//-将文件内部的位置指针重新指向一个流（数据流/文件）的开头
        if (nrts == hbstamps) {
            // write from the logical start of the buffer to the physical end
            if (fwrite(timestamps + (lastts * TS_SIZE), TS_SIZE, hbstamps - lastts, hb) == 0) {
		    int err = errno;
#if USE_SYSLOG
		    syslog(LOG_ERR, "write heartbeat file gave error %d = '%m'!", err);
#else			/* USE_SYSLOG */
		    perror(progname);
#endif			/* USE_SYSLOG */
	    }
	    
            // write from the physical start of the buffer to the logical end
            if (fwrite(timestamps, TS_SIZE, lastts, hb) == 0) {
		    int err = errno;
#if USE_SYSLOG
		    syslog(LOG_ERR, "write heartbeat file gave error %d = '%m'!", err);
#else			/* USE_SYSLOG */
		    perror(progname);
#endif			/* USE_SYSLOG */
	    }
        }
        else {        
            // write from the physical start of the buffer to the logical end
            if (fwrite(timestamps, TS_SIZE, nrts, hb) == 0) {	//-向指定的文件中写入若干数据块
		    int err = errno;
#if USE_SYSLOG
		    syslog(LOG_ERR, "write heartbeat file gave error %d = '%m'!", err);
#else			/* USE_SYSLOG */
		    perror(progname);
#endif			/* USE_SYSLOG */
	    }
        }
        fflush(hb);   
    } 
    return(ENOERR);
}

/* write to the watchdog device */
int keep_alive(void)	//-进行了喂狗,并填写了一个心跳文件
{
    if (watchdog == -1)
	return (ENOERR);

    if (write(watchdog, "\0", 1) < 0) {	//-喂狗
	int err = errno;

#if USE_SYSLOG
	syslog(LOG_ERR, "write watchdog device gave error %d = '%m'!", err);
#else			/* USE_SYSLOG */
	perror(progname);
#endif			/* USE_SYSLOG */
	if (softboot)
	    return (err);
    }
    
    /* MJ 20/2/2001 write a heartbeat to a file outside the syslog, because:
       - there is no guarantee the system logger is up and running
       - easier and quicker to parse checkpoint information */
    write_heartbeat();	//-写心跳文件
    
    return(ENOERR);
}
