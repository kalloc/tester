#include <stdio.h>
#include <time.h>
#include <sys/time.h>

#define ITER 1000

#define bench_start() s=rdtsc();
#define bench_end(str) e=rdtsc();printf("%015s\t\t%lld\n",str,(e-s-112));
unsigned long long e,s;

int timeDiffUS(struct timeval end, struct timeval start) {
    return (int) ((((double) end.tv_sec + (double) end.tv_usec / 1000000) - ((double) start.tv_sec + (double) start.tv_usec / 1000000))*1000000);
}
        
static struct timeval tv;
inline static long long rdtsc(void) {
    long long l;
    asm volatile("rdtsc\r\n" : "=A" (l));
    return l;
}
        
int getMS() {
    register long long rd,rTmp;
    static struct timeval tv;
    rTmp=rdtsc()/100000;
    if(rd != rTmp) {
        gettimeofday(&tv,NULL);
        rd=rTmp;
    }
    return tv.tv_sec*1000+tv.tv_usec/1000;
}


inline int getMS2() {
    static struct timespec time;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &time);
    return time.tv_nsec;
}

unsigned long long (*const rdts)()=(unsigned int(*)())(char*)"\x0f\x31\xc3";

int main() {
#define ITER 1000
    struct timeval start,end;

    unsigned int i;

    gettimeofday(&tv,NULL);
    bench_start() 
    bench_end("nop");


    bench_start() 
    for(i=0;i<ITER;i++) {
	gettimeofday(&tv,NULL);
    }
    bench_end("gettimeofday");
    bench_start() 
    for(i=0;i<ITER;i++) {
	rdtsc();
    }
    bench_end("rdtsc");

    bench_start() 
    for(i=0;i<ITER;i++) {
	getMS();
    }
    bench_end("getMS()");

    bench_start() 
    for(i=0;i<ITER;i++) {
	getMS2();
    }
    bench_end("getMS2()");

}

