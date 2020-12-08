#include "sleep.h"
#include <time.h>

int nsleep(long ns) {
    struct timespec req={0};
    time_t sec=(int)(ns/1000000000L);
    ns=ns-(sec*1000000000L);
    req.tv_sec=sec;
    req.tv_nsec=ns;
    while(nanosleep(&req,&req)==-1)
         continue;
    return 0;
}

