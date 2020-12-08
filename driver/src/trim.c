#include "trim.h"
#include <string.h>
#include <ctype.h>

/* Remove leading whitespaces */
char *ltrim(char *s) {
	if(s && *s) {
		char *cur = s;
		while(*cur && isspace(*cur))
			++cur;

		if(s != cur)
			memmove(s, cur, strlen(cur) + 1);
	}
	return s;
}
/* Remove trailing whitespaces */
char *rtrim(char *s) {
	if(s && *s) {
		char *cur = s + strlen(s) - 1;
		while(cur != s && isspace(*cur))
			--cur;

		cur[isspace(*cur) ? 0 : 1] = '\0';
	}
	return s;
}

/* Remove leading and trailing whitespaces */
char *trim(char *s) {
	rtrim(s);
	ltrim(s);
	return s;
}
