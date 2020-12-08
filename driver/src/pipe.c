#include "pipe.h"

#include "trim.h"

#include <string.h>
#include <fcntl.h> // opening files
#include <unistd.h> // closing and removing files

int read_pipe(int fd, char *buf, int buf_len) {
	int res;
	memset(buf, 0 ,buf_len);
	res = read(fd, buf, buf_len);
	if ( res > 0 ) {
		strncpy(buf, trim(buf), buf_len);
		return strlen(buf);
	}
	return 0;
}

int write_pipe(int fd, char *buf) {
	return write(fd, buf, strlen(buf));
}
