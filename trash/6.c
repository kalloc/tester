#include <stdio.h>

int main () {
    FILE *src;
    char buf[4096];
    int len;
    src=fopen("/etc/passwd","r");
    len=fread(buf,1,4096,src);
    fclose(src);
    printf("readed %d byte\n",len);


}
