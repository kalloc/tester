#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <zlib.h>

#define BUFLEN 16384
int compress2  (Bytef *dest, uLongf *destLen, const Bytef *source, uLong sourceLen, int level);
int uncompress (Bytef *dest, uLongf *destLen, const Bytef *source, uLong sourceLen);

typedef struct {
    char *ptr;
    u_int len;
} Data;

Data makeData(char *ptr,u_int len)  {
    static Data data;
    data.ptr=ptr;
    data.len=len;
    return data;
}

Data Zlib_Compress (char *ptr,u_int len) {
    char static Out[BUFLEN];
    u_int static OutLen=BUFLEN;
    compress2 ((Bytef *)&Out, (uLongf *)&OutLen, (Bytef *)ptr, len, Z_DEFAULT_COMPRESSION);
    return makeData((char *)&Out,OutLen);
}
Data Zlib_Decompress (char *ptr,u_int len) {
    char static Out[BUFLEN];
    u_int static OutLen=BUFLEN;
    uncompress ((Bytef *)&Out, (uLongf *)&OutLen, (Bytef *)ptr, len);
    return makeData((char *)&Out,OutLen);
}


ulong inline checksum(u_char * buffer,u_int length) {
    return crc32(0L, (const Bytef *)buffer,length) & 0xffffffff;
}





#ifdef TEST

/* compress or decompress from stdin to stdout */
int main(int argc, char **argv)
{
    char Plain[]=
	"Some Some Chars"
    ;

    Data ptr;
    printf("IN C:   0x%08x\n",checksum((unsigned char *)Plain,sizeof(Plain)));

    ptr=Zlib_Compress(Plain,sizeof(Plain));
    ptr=Zlib_Decompress(ptr.ptr,ptr.len);
//    printf("strcmp(Plain,Decrypt) =  %d 0x%08x %d-%d\n",strcmp(ptr.ptr,Plain),checksum(ptr.ptr,ptr.len),sizeof(Plain),ptr.len);
    return 0;
}

#endif#include <stdio.h>
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
