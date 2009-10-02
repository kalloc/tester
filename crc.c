#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <zlib.h>


static inline long long rdtsc(void) {
    long long l;
    asm volatile("rdtsc\r\n":"=A" (l));
    return l;
}


ulong inline checksum(u_char * buffer,u_int length) {
    return crc32(0L, (const Bytef *)buffer,length) & 0xffffffff;
}




/* compress or decompress from stdin to stdout */
int main(int argc, char **argv)
{
    u_char Plain[]="Some Some Chars";
    int i=0;
    #define CYCLE 1000000
    long long start = 0;
    initCrc32();
    start=rdtsc();
    for(i=0;i<CYCLE;i++) {
	checksum(Plain,sizeof(Plain)-1)+1;
    }
    printf("ts=%lu\n",(rdtsc()-start));
    start=rdtsc();
    for(i=0;i<CYCLE;i++) {
	Crc32(Plain,sizeof(Plain)-1)+1;
    }
    printf("ts=%lu\n",(rdtsc()-start));

    //printf("IN C:\t\t0x%08x\n",checksum(Plain,strlen(Plain)));
    return 0;
}
