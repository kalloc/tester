#include <stdio.h>
#include <stdlib.h>
#include <iconv.h>

int main() {
    iconv_t cd;
    char * fromString, *toString;
    char **fromStringPtr,**toStringPtr;
    size_t lenIn , lenOut ;
    int i;
    FILE * fd = fopen("/tmp/buf.","r");
    lenIn = fread(fromString,lenIn,1,fd);
    fclose(fd);
    lenOut = lenIn * 4;
    toString = calloc(1,lenOut);
    fromStringPtr = &fromString;
    toStringPtr = &toString;
    cd = iconv_open((const char *)"UTF-8",(const char *) "WINDOWS-1251");
    printf("1. to %p %p\n",toStringPtr,toString);
    printf("1. from %p %p\n",fromStringPtr,fromString);
    i = iconv(cd,
            &fromString, &lenIn,
            &toString, &lenOut
            );
    printf("2. to %p %p\n",toStringPtr,toString);
    printf("2. from %p %p\n",fromStringPtr,fromString);
    iconv_close(cd);
    printf("3. to %p %p\n",toStringPtr,toString);
    printf("3. from %p %p\n",fromStringPtr,fromString);
    free(toString);
    printf("4. to %p %p\n",toStringPtr,toString);
    printf("4. from %p %p\n",fromStringPtr,fromString);
}