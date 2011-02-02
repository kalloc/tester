#include <stdio.h>
#include <stdlib.h>
#include <zlib.h>

#pragma pack(1)

void hexPrint(char *aMsg, int aLen) {
#define LINE_LEN 16
    int sOffset = 0;
    int lIdx;
    for (lIdx = 0; aLen > 0; aLen -= LINE_LEN, sOffset += LINE_LEN) {
        /*
         * Print the offset
         */
        printf("  %05d: ", sOffset);

        /*
         * The Hex data
         */
        for (lIdx = 0; lIdx < LINE_LEN; lIdx++) {
            if (lIdx < aLen) {
                printf("%02x ", (unsigned char) aMsg[lIdx + sOffset]);
            } else {
                printf("   ");
            }
        }

        printf(" : ");

        /*
         * The ASCII data
         */
        for (lIdx = 0; lIdx < LINE_LEN; lIdx++) {
            char lChar = ' ';

            if (lIdx < aLen) {
                lChar = aMsg[lIdx + sOffset];
            }
            printf("%c", (((lChar & 0x80) == 0) && isprint((int) lChar)) ? lChar : '.');
        }

        printf("\n");

    }
}

//конец спизженного куска кода, мне стыдно (:
#define u32 unsigned int

    struct _st_sizes {
        u32 CmprSize; // Размер блока данных в сжатом виде, или 0, если данные несжаты
        u32 UncmprSize; // Размер блока данных
        u32 crc; // crc32()^0xFFFFFFFF блока данных
    } sizes;

    struct _st_t_hdr {
        u32 TesterId; // ID тестера
        u32 ReqType; // Код команды/moduleid

    } hdr;

struct Request {
    //1. Описатель длины/целостности пакета
    struct _st_sizes sizes;
    //2. Заголовок данных
    struct _st_t_hdr hdr;
    char *Data;
};


struct One {
    char One;
    char Two;
};


int main () {
    struct Request *req;
    
    char * ptr = malloc(4096);
    bzero(ptr,4096);
    char * ptrCmpr = malloc(4096);
    bzero(ptrCmpr,4096);
    char * ptr2Cmpr = malloc(4096);
    bzero(ptr2Cmpr,4096);
    
    int len = sizeof(*req);
    int lenCmpr = 4096;
    int len2Cmpr = 4096;
    req = (struct Request *) (ptr);
    req->sizes.UncmprSize=len-sizeof(req->sizes);
    req->hdr.TesterId = 1;
    req->hdr.ReqType = 1000;

    printf("-------------------------------------------\n");
    printf("sizeof struct Request: %d\n",sizeof(*req));
    hexPrint(ptr, len);
    printf("sizeof struct Request -> sizes: %d\n",sizeof((req->sizes)));
    hexPrint(ptr, sizeof (req->sizes));

    printf("sizeof struct Request -> hdr: %d\n",sizeof((req->hdr)));
    hexPrint(ptr + sizeof (req->sizes), sizeof (req->hdr));
    printf("-------------------------------------------\n\n");



    printf("Compress Size:%d\n",len - sizeof (req->sizes));
    hexPrint(ptr + sizeof (req->sizes),len - sizeof (req->sizes));
    printf("-------------------------------------------\n");
    printf("compress ret:%d\n",compress2((Bytef *) ptrCmpr, (uLongf *) & lenCmpr, (Bytef *)  ptr + sizeof (req->sizes), len - sizeof (req->sizes), Z_DEFAULT_COMPRESSION));
    printf("Compressed Size:%d\n",lenCmpr);
    hexPrint(ptrCmpr,lenCmpr);
    printf("-------------------------------------------\n");
    printf("uncompress ret:%d\n",uncompress((Bytef *) ptr2Cmpr, (uLongf *) & len2Cmpr, (Bytef *) ptrCmpr, lenCmpr));
    printf("Decompressed Size:%d\n",len2Cmpr);
    hexPrint(ptr2Cmpr,len2Cmpr);


    memcpy(ptr + sizeof (req->sizes), ptrCmpr, lenCmpr);
    printf("\n-------------------------------------------\n");
    printf("sizeof struct Request: %d\n",sizeof(*req)+lenCmpr);
    hexPrint(ptr, len);

    printf("sizeof struct Request -> sizes: %d\n",sizeof((req->sizes)));
    hexPrint(ptr, sizeof (req->sizes));

    printf("sizeof struct Request -> data: %d\n",lenCmpr);
    hexPrint(ptr+sizeof(req->sizes), lenCmpr);
    struct One *pone;

}
//EOF
