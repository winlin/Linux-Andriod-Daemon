#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
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
#define LOCAL_TIME_STR_LEN	60

/* The defination for task */
#define COMMANDLINE_MAX_LENGTH	100
typedef struct task{
	struct task *next;
	char commandline[COMMANDLINE_MAX_LENGTH];
}task;
/* The task queue */
typedef struct taskqueue{
	task *head;
	task *tail;
	int count;
}taskqueue;
/* Queue operate */
void initqueue(taskqueue *tqueue)
{
	tqueue->head = NULL;
	tqueue->tail = NULL;
	tqueue->count = 0;
}
void pushqueue(taskqueue *tqueue, task *tsk)
{
	if(tqueue->count == 0) {
		tqueue->head = tsk;
	} else {
		tqueue->tail->next = tsk;
	}

	tqueue->tail = tsk;
	tqueue->count++;
}
task *popqueue(taskqueue *tqueue)
{
	if(tqueue->count == 0)
		return NULL;

	task *tm = tqueue->head;
	tqueue->count--;
	if(tqueue->count == 0)
		tqueue->head = NULL;
	else
		tqueue->head = tm->next;

	return tm;
}
task *topqueue(taskqueue *tqueue)
{
	return tqueue->head;
}

/* Tools func */
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

/* Commit a task*/
void committask(task *tmtask)
{
	WS_dputs("call one time");
	if(tmtask == NULL)
		return;

	pid_t pid = fork();
	if(pid < 0) {
		WS_dprintf("call fork() failed! errno:%d errmsg:%s", errno, strerror(errno));
	} else if(pid > 0) {
		WS_dprintf("pid:%d  EXEC:%s", pid, tmtask->commandline);
	} else if(pid == 0){
		//commit the task
		FILE *execfp = NULL;
		char buf[16] = {0};
		int read = 0;
		execfp = popen(tmtask->commandline, "r");
		while((read=fread(buf, sizeof(char), 15, execfp))!= 0) {
			printf("%s", buf);
			memset(buf, 0, 16);
		}
		free(tmtask);
		tmtask = NULL;
		exit(EXIT_SUCCESS);
	}
}

int main(int argc, char* argv[])
{
	if(argc != 3) {
		printf("Usage: hello -cmd commandline\n");
		exit(EXIT_FAILURE);
	}
	taskqueue *execqueue = (taskqueue *)calloc(0, sizeof(taskqueue));
	if(execqueue == NULL) {
		WS_dputs("call calloc() failed! create a task queue failed");
		exit(EXIT_FAILURE);
	}
	initqueue(execqueue);

	//convert into a daemon
	int ret = daemon(1, 1);
	if(ret == -1) {
		WS_dprintf("call daemon() failed! errno:%d errmsg:%s", errno, strerror(errno));
	}
	WS_dprintf("daemon pid:%d \n", getpid());
	//add a task
	task *tmtask = (task *)calloc(0, sizeof(task));
	if(tmtask == NULL) {
		WS_dputs("call calloc() failed! create a task failed");
		exit(EXIT_FAILURE);
	} else {
		strncpy(tmtask->commandline, argv[2], COMMANDLINE_MAX_LENGTH);
		pushqueue(execqueue, tmtask);
	}

    while (1)
    {
        //dont block context switches, let the process sleep for some time
        sleep(1);
        writemsg(stdout, "@\n");
        // implement and call some function that does core work for this daemon.
        if(execqueue->count > 0) {
        	task *tm = popqueue(execqueue);
        	committask(tm);
        }
        //clean the finished child process(del zombie)
        waitpid(-1, NULL, WNOHANG);
    }
    return (0);
}
