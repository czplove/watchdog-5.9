/* $Header: /cvsroot/watchdog/watchdog/src/iface.c,v 1.2 2006/07/31 09:39:23 meskes Exp $ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include "extern.h"
#include "watch_err.h"

#if USE_SYSLOG
#include <syslog.h>
#endif

#define NETDEV_LINE_LEN	128

int check_iface(struct list *dev)	//-判断网络是否有数据被接收到,并根据实际情况记录系统日志
{
    FILE *file = fopen ("/proc/net/dev", "r");
    
    if (file == NULL) {
        int err = errno;
        
#if USE_SYSLOG
        syslog(LOG_ERR, "cannot open /proc/net/dev (errno = %d = '%m')", err);
#else                           /* USE_SYSLOG */
        perror(progname);
#endif                          /* USE_SYSLOG */

        if (softboot)
		return (err);

	return(ENOERR);
    }

    /* read the file line by line */
    while (!feof(file)) {	//-检测流上的文件结束符，如果文件结束，则返回非0值，否则返回0
	char line[NETDEV_LINE_LEN];
        
        if (fgets(line, NETDEV_LINE_LEN, file) == NULL) {	//-从文件结构体指针stream中读取数据，每次读取一行。函数成功将返回buf，失败或读到文件结尾返回NULL。
		if (!ferror(file))
		        break;
	        else {
	        	int err = errno;
#if USE_SYSLOG
		        syslog(LOG_ERR, "cannot read /proc/net/dev (errno = %d = '%m')", err);
#else                           /* USE_SYSLOG */
		        perror(progname);
#endif                          /* USE_SYSLOG */

			fclose(file);
		        if (softboot)
				return (err);

			return(ENOERR);
	        }
	} else {
		int i = 0;
			
		for (; line[i] == ' ' || line[i] == '\t'; i++);
		if (strncmp(line + i, dev->name, strlen(dev->name)) == 0) {
			unsigned long bytes = strtoul(line + i + strlen(dev->name) + 1, NULL, 10);	//-将字符串转换成无符号长整型数
			
#if USE_SYSLOG
			/* do verbose logging */
			if (verbose && logtick && ticker == 1)
		            syslog(LOG_INFO, "device %s received %lu bytes", dev->name, bytes);
#endif   

			if (dev->parameter.iface.bytes == bytes) {
				fclose(file);
#if USE_SYSLOG
		            	syslog(LOG_ERR, "device %s did not receive anything since last check", dev->name);
#endif   

				return (ENETUNREACH);
			}
			else
				dev->parameter.iface.bytes = bytes;	//-记录读取到的设备接收字节数
		}
        }
    }

    if (fclose(file) != 0) {
        int err = errno;
        
#if USE_SYSLOG
        syslog(LOG_ERR, "cannot close /proc/net/dev (errno = %d = '%m')", err);
#else                           /* USE_SYSLOG */
        perror(progname);
#endif                          /* USE_SYSLOG */

        if (softboot)
		return (err);

	return(ENOERR);
    }

    return (ENOERR);
}
