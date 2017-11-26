/*************************************************************/
/* Small utility to identify hardware watchdog               */
/* 							     */
/* Idea and most of the implementation by		     */
/* Corey Minyard <minyard@acm.org>			     */
/*                                                           */
/* The rest was written by me, Michael Meskes                */
/* meskes@debian.org                                         */
/*                                                           */
/*************************************************************/
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <errno.h>
#include <stdio.h>
#include <linux/watchdog.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <libgen.h>
#include <getopt.h>

#define DEVICE		"watchdog-device"	//-设备名,可以在配置文件里面获取到这个参数值

int watchdog = -1;
char *devname = NULL, *progname = NULL;

static void usage(void)
{
    fprintf(stderr, "%s version %d.%d, usage:\n", progname, MAJOR_VERSION, MINOR_VERSION);
    fprintf(stderr, "%s \n", progname);
    exit(1);
}

void terminate(void)	//-终端
{
	if (watchdog != -1) {
	    if (write(watchdog, "V", 1) < 0 )	//-关闭看门狗
		perror(progname);

	    if (close(watchdog) == -1)
		perror(progname);
	}

	exit(0);
}

static int spool(char *line, int *i, int offset)	//-找到需要的位置
{
    for ( (*i) += offset; line[*i] == ' ' || line[*i] == '\t'; (*i)++ );	//-去掉空格
    if ( line[*i] == '=' )
        (*i)++;
    for ( ; line[*i] == ' ' || line[*i] == '\t'; (*i)++ );	//-再次去掉空格
    if ( line[*i] == '\0' )
        return(1);
    else
        return(0);
}

static void read_config(char *configfile, char *progname)	//-读指定配置文件中指定的选项参数
{
    FILE *wc;

    if ( (wc = fopen(configfile, "r")) == NULL ) {
        perror(progname);
        exit(1);
    }

    while ( !feof(wc) ) {	//-读文件内容直到结束
	char *line = NULL;
	size_t n;

	if (getline(&line, &n, wc) == -1) {	//-读取整行文本
            if ( !ferror(wc) )
                break;
            else {
                perror(progname);
                exit(1);
            }
        }
        else {
            int i, j;

            /* scan the actual line for an option */
            /* first remove the leading blanks */
            for ( i = 0; line[i] == ' ' || line[i] == '\t'; i++ );

            /* if the next sign is a '#' we have a comment */
            if ( line[i] == '#' )	//-跳过注释
                continue;

            /* also remove the trailing blanks and the \n */
            for ( j = strlen(line) - 1; line[j] == ' ' || line[j] == '\t' || line[j] == '\n'; j-- );
            line[j + 1] = '\0';

            /* if the line is empty now, we don't have to parse it */
            if ( strlen(line + i) == 0 )	//-跳过空行
                continue;

            /* now check for an option */
            if ( strncmp(line + i, DEVICE, strlen(DEVICE)) == 0 ) {	//-若str1与str2的前n个字符相同，则返回0
                if ( spool(line, &i, strlen(DEVICE)) )
                    devname = NULL;
                else
                    devname = strdup(line + i);	//-将字符串拷贝到新建的位置处(在内部调用了malloc()为变量分配内存)
	    } 
            else {
                /*
		 * do not print an error message here because we usually use
		 * watchdog's config file which may contain far more valid
		 * options than we understand 
		 */
		/* fprintf(stderr, "Ignoring config line: %s\n", line); */
            }
        }
    }

    if ( fclose(wc) != 0 ) {
        perror(progname);
        exit(1);
    }
}

//- ./wd_identify
int main(int argc, char *const argv[])	//-主要根据配置文件识别了看门狗,并没有实际周期操作
{
    FILE *fp;
    char *configfile = CONFIG_FILENAME;
    char *filename_buf;
    int count = 0;
    int c;
    struct watchdog_info ident;
    char *opts = "c:";
    struct option long_options[] =
    {
	{"config-file", required_argument, NULL, 'c'},	//-长参数名 表示参数后面是否跟数值 用来决定，getopt_long()的返回值到底是什么 /和flag联合决定返回值
	{NULL, 0, NULL, 0}
    };

    progname = basename(argv[0]);	//-获取文件名

    /* check for the one option we understand */
    while ((c = getopt_long(argc, argv, opts, long_options, NULL)) != EOF) {
	if (c == -1)
	    break;
	switch (c) {
	case 'c':
	    configfile = optarg;
	    break;
	default:
	    usage();
	}
    }

    read_config(configfile, progname);

    /* this program has no other function than iidentifying the hardware behind
     * this device i.e. if there is no device given we better punt */
    if ( devname == NULL )	//-这个数值是在读配置文件的时候获取的记得需要free否则内存泄漏
	exit(0);

    /* open the device */
    watchdog = open(devname, O_WRONLY);	//-打开看门狗设备,这个名字可以通过配置文件变化
    if ( watchdog == -1 ) {
            perror(progname);
            exit(1);
    }

    /* Print watchdog identity */
    if (ioctl(watchdog, WDIOC_GETSUPPORT, &ident) < 0) {	//-读板卡信息
	perror(progname);
    }
    else {
	ident.identity[sizeof(ident.identity) - 1] = '\0'; /* Be sure */
	printf("%s\n", ident.identity);
    }

    if (write(watchdog, "V", 1) < 0 )	//-关闭看门狗
	perror(progname);

    if (close(watchdog) == -1)	//-关闭文件
	perror(progname);

    exit(0);
}


