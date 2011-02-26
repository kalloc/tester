#include <stdio.h>
#include <stdlib.h>

int main() {
#define LEN 255
    char **p;
    int i;
    p=calloc(1,sizeof(char *)*(LEN+1));
    for(i=0;i<LEN;i++) {
	p[i]=malloc(LEN);
	snprintf(p[i],LEN,"%d",i);
    }
    char **ptr;
    for(ptr=p;*ptr;ptr++) {
	printf("%s\n",*ptr);
    }


}
