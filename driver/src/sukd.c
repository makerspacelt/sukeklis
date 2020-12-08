#include "sukd.h"

#include "sleep.h"
#include "trim.h"
#include "pipe.h"
#include "libsuk.h"

#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h> // opening files
#include <unistd.h> // closing and removing files
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <pthread.h>
#include <syslog.h>
#include <signal.h>

#define CMD_PIPE "/tmp/command"
#define STAT_FILE "/tmp/status"
#define LOG_FILE "/tmp/log"
#define BS 1024

volatile sig_atomic_t finish = 0;


int main(int argc, char *argv[]) {
	pthread_t t_status, t_commands, t_sukeklis;
	int sid;


	// start as daemon
	if (getuid() > 0) {
		puts("This program must be run under user root!");
		return -1;
	}

	/* Fork off the parent process */
/*
	pid_t pid;
	pid = fork();
	if (pid < 0) {
		puts("[FAIL] failed to fork");
		exit(EXIT_FAILURE);
	}
	// If we got a good PID, then
	// we can exit the parent process.
	if (pid > 0) {
		exit(EXIT_SUCCESS);
	}
*/

	// change umask
	umask(0);

	// loging stuff
	setlogmask (LOG_UPTO (LOG_DEBUG));
	openlog("sukeklis", LOG_PID, LOG_USER);
	syslog(LOG_INFO,"Logging started.");


	// session id
	sid = setsid();
	if (sid < 0) {
		syslog(LOG_ERR,"[FAIL] failed to setsid()");
		exit(EXIT_FAILURE);
	}

	// change working dir
	if ((chdir("/tmp/")) < 0) {
		syslog(LOG_ERR,"[FAIL] failed chdir() to /tmp/");
		exit(EXIT_FAILURE);
	}


	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);

	// register SIGTERM
	signal (SIGTERM, catch_terminate);


	if ( pthread_create( &t_status, NULL, tstatus, NULL) ) {
		syslog(LOG_ERR,"error creating status thread.");
		return -1;
	}
	if ( pthread_create( &t_commands, NULL, tcommands, NULL) ) {
		syslog(LOG_ERR,"error creating commands thread.");
		return -1;
	}
	if ( pthread_create( &t_sukeklis, NULL, tsukeklis, NULL) ) {
		syslog(LOG_ERR,"error creating sukeklis thread.");
		return -1;
	}


	syslog(LOG_INFO,"entering main loop.");
	while ( finish < 1 ) {
		nsleep(100*1000*1000);
	}
	syslog(LOG_INFO,"main loop ended.");


	if ( pthread_join ( t_status, NULL ) ) {
		syslog(LOG_ERR,"error joining status thread.");
		return -1;
	}
	if ( pthread_join ( t_commands, NULL ) ) {
		syslog(LOG_ERR,"error joining commands thread.");
		return -1;
	}
	if ( pthread_join ( t_sukeklis, NULL ) ) {
		syslog(LOG_ERR,"error joining sukeklis thread.");
		return -1;
	}

	syslog(LOG_INFO,"END logging");
	closelog();
	return(0);
}

/*
 * returns:
 * 0 if command not found,
 * 1 if command found,
 * 2 if command and param found.
 */
int parse_command(char *buf, char *cmd, char *param, int len) {
	memset(cmd, 0 , len);
	memset(param, 0 , len);
	strncpy(buf, trim(buf), len);

	if ( strlen(buf) == 0 ) return 0;

	// find first whitespace
	int i;
	for (i=0; i<=strlen(buf); i++) {
		if ( *(buf+i) && isspace(*(buf+i)) ) {
			break;
		}
	}

	strncpy(cmd, buf, i);
	if ( strlen(buf) > i )
		strncpy(param, (buf+i), strlen(buf)-i );

	strncpy(cmd, trim(cmd), len);
	strncpy(param, trim(param), len);

	return (strlen(param)>0)?2:1;
}
void catch_terminate(int sig) {
	finish = 1;
}
void *tstatus(void *arg) {
	int res,stat_fd;
	char buf[BS];

	unlink(STAT_FILE);
	stat_fd = open( STAT_FILE, O_CREAT|O_WRONLY|O_TRUNC|O_NONBLOCK, 0644);
	if (stat_fd < 0) syslog(LOG_ERR,"opening %s status file returned %d\n", STAT_FILE, stat_fd);

	while ( finish < 1) {
		res = libsuk_status(buf, BS);
		res = ftruncate(stat_fd, 0);
		res = lseek(stat_fd, 0, SEEK_SET);
		res = write_pipe(stat_fd, buf);
		res++;
		nsleep(100*1000*1000);
	}

	close(stat_fd);
	unlink(STAT_FILE);

	return NULL;
}
void *tcommands(void *arg) {
	int res,cmd_fd;
	char buf[BS], cmd[BS], param[BS];

	unlink(CMD_PIPE);
	res = mkfifo( CMD_PIPE, 0622 );
	if (res != 0) syslog(LOG_ERR,"making %s fifo returned %d\n", CMD_PIPE, res);
	cmd_fd = open( CMD_PIPE, O_RDONLY|O_NONBLOCK );
	if (cmd_fd < 0) syslog(LOG_ERR,"opening %s fifo returned %d\n", CMD_PIPE, cmd_fd);

	while ( finish < 1) {
		res = read_pipe(cmd_fd, buf, BS);
		if (res > 0) {
			// parse command and param
			res = parse_command(buf, cmd, param, BS);
			// do stuff
			if ( strcmp(cmd,"target") == 0 ) {
					syslog(LOG_INFO,"command: %s(%.5f)",cmd,strtod(param, NULL));
					libsuk_set_target( strtod(param, NULL) );

			} else if ( strcmp(cmd,"relative") == 0 ) {
					syslog(LOG_INFO,"command: %s(%.5f)",cmd,strtod(param, NULL));
					libsuk_set_target( status.target + strtod(param, NULL) );

			} else if ( strcmp(cmd,"pause") == 0 ) {
					syslog(LOG_INFO,"command: %s()",cmd);
					status.pause = 1;

			} else if ( strcmp(cmd,"resume") == 0 ) {
					syslog(LOG_INFO,"command: %s()",cmd);
					status.pause = 0;

			} else if ( strcmp(cmd,"spin") == 0 ) {
					syslog(LOG_INFO,"command: %s(%d)",cmd,atoi(param));
					status.spin = 1;
					status.spin_dir = atoi(param) & 1;

			} else if ( strcmp(cmd,"stop") == 0 ) {
					syslog(LOG_INFO,"command: %s()",cmd);
					status.stop = 1;

			} else if ( strcmp(cmd,"acc") == 0 ) {
					syslog(LOG_INFO,"command: %s(%.5f)",cmd,strtod(param, NULL));
					status.acc = strtod(param, NULL);

			} else if ( strcmp(cmd,"min_sleep") == 0 ) {
					syslog(LOG_INFO,"command: %s(%ld)",cmd,atol(param));
					status.min_sleep = atoi(param);

			} else if ( strcmp(cmd,"max_sleep") == 0 ) {
					syslog(LOG_INFO,"command: %s(%ld)",cmd,atol(param));
					status.max_sleep = atoi(param);

			} else if ( strcmp(cmd,"res") == 0 ) {
					syslog(LOG_INFO,"command: %s(%d)",cmd,atoi(param));
					libsuk_res_set( atoi(param) );

			} else {
					syslog(LOG_WARNING,"unknown cmd(%s); param(%s)\n", cmd, param);
			}
		}
		nsleep(100*1000*1000);
	}

	close(cmd_fd);
	unlink(CMD_PIPE);

	return NULL;
}
void *tsukeklis(void *arg) {
	libsuk_init();

	libsuk_go(&finish);
	
	return NULL;
}