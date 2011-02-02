#include <stdio.h>
#include <stdlib.h>
#include <time.h>

inline int timeDiffMS(struct timeval *end,struct timeval *start) {
    return (int)((((double) end->tv_sec + (double)end->tv_usec/1000000) - ((double)start->tv_sec+ (double)start->tv_usec/1000000))*1000);
}


inline int timeDiffUS(struct timeval *end,struct timeval *start) {
    return (int)((((double) end->tv_sec + (double)end->tv_usec/1000000) - ((double)start->tv_sec+ (double)start->tv_usec/1000000))*1000000);
}

int main() {
    struct timeval tv1;
    struct timeval tv2;
    struct timespec ts;
    gettimeofday(&tv1,NULL);
    ts.tv_sec = 0;
    ts.tv_nsec = 100*1000*1000;
    nanosleep(&ts,NULL);
    gettimeofday(&tv2,NULL);
    printf("us diff %d\n",timeDiffUS(&tv2,&tv1));
    //us diff 100059
    printf("ms diff %d\n",timeDiffMS(&tv2,&tv1));
    //ms diff 100

    return 0;
}

