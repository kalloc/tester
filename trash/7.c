#include <stdio.h>
#include <mcheck.h>

int main()
{
char *ptr;
ptr=malloc(40096);
ptr=malloc(500);
free(ptr);
}
