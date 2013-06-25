/*
 * This demo just call popen() to exec app, but the 
 * daemon is stop when the app was execed.
 * */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>

#define WS_DEBUG
#ifdef WS_DEBUG
#define WS_dputs(str) printf("%s %d:  %s\n", __func__, __LINE__, str)
#define WS_dprintf(fmt, args...) printf("%s %d:  "fmt"\n", __func__, __LINE__, ##args)
#else
#define WS_dputs(str)
#define WS_dprintf(fmt, args...)
#endif

#define DAEMON_PATH         "/"
#define LOG_FILE_PATH_STR   "/data/"
#define LOG_FILE_NAME_PRE   "BusSale.log"
#define LOG_EXEC_CMD_NAME	"BSExec.log"
#define LOCAL_TIME_STR_LEN	60

void getlocaltime(char *timestr)
{
    time_t t;
    struct tm *tmp;
    t = time(NULL);
    tmp = localtime(&t);
    strftime(timestr, LOCAL_TIME_STR_LEN, "%F %a %T", tmp);
    return ;
}

int writemsg(FILE *fp, char * msgstr)
{
	char timestr[LOCAL_TIME_STR_LEN] = {0};
	getlocaltime(timestr);
	int ret = fprintf(fp, "%s %s", timestr, msgstr);
    return ret;
}

int main(int argc, char* argv[])
{
	if(argc != 3) {
		printf("Usage: hello -cmd commandline\n");
		exit(EXIT_FAILURE);
	}
	int ret = daemon(1, 1);
	if(ret == -1) {
		WS_dprintf("call daemon() failed! errno:%d errmsg:%s", errno, strerror(errno));
	}

	FILE *execfp = NULL;
	char buf[16] = {0};
	int read = 0;
	execfp = popen(argv[2], "r");
	while((read=fread(buf, sizeof(char), 15, execfp))!= 0) {
		printf("%s", buf);
		memset(buf, 0, 16);
	}

    while (1)
    {
        //Dont block context switches, let the process sleep for some time
        sleep(3);
        writemsg(stdout, "one rotate\n");
         // Implement and call some function that does core work for this daemon.
    }
    return (0);
}

