#include <stdio.h>
#include <stdlib.h>
#include <iconv.h>

int main() {
    iconv_t cd;
    char * fromString, *toString;
    size_t lenIn , lenOut ;
    int i;
    FILE * fd = fopen("/tmp/buf.","r");
    fromString = malloc(40000);
    lenIn = fread(fromString,40000,1,fd);
    fclose(fd);
    lenOut = 0;
    toString = calloc(1,400000);
    cd = iconv_open((const char *)"UTF-8//IGNORE",(const char *) "WINDOWS-1251");
    printf("1. to %p\n",toString);
    printf("1. from %p\n",fromString);
    i = iconv(cd,
            &fromString, &lenIn,
            &toString, &lenOut
            );
    printf("1. to %p\n",toString);
    printf("1. from %p\n",fromString);
    printf("lenOut %d %d\n",lenOut,i);
    printf("%s\n",toString);
    iconv_close(cd);
    free(toString);
    free(fromString);

}