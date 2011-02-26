#define _GNU_SOURCE     /* Expose declaration of tdestroy() */
#include <search.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>


struct Bla {
    int id;
    char buf[20];
    enum {BLA,BLA2,BLA3,BLA4} type;
};
void *root = NULL;

	void *
xmalloc(unsigned n)
{
	void *p;
	p = malloc(n);
	if (p)
		return p;
	fprintf(stderr, "insufficient memory\n");
	exit(EXIT_FAILURE);
}

	int
compare(const void *pa, const void *pb)
{
	if(((struct Bla *)pa)->id > ((struct Bla *)pb)->id) {
	    return 1;
	} 
	if(((struct Bla *)pa)->id < ((struct Bla *)pb)->id) {
	    return -1;
	} 
	return 0;
}

	void
action(const void *nodep, const VISIT which, const int depth)
{
	int *datap;

	switch (which) {
		case preorder:
			break;
		case postorder:
			datap = *(int **) nodep;
			printf("%6d\n", *datap);
			break;
		case endorder:
			break;
		case leaf:
			datap = *(int **) nodep;
			printf("%6d\n", *datap);
			break;
	}
}

	int
main(void)
{
	int i;
	void *val;

	struct Bla *bla;
	srand(time(NULL));
	for (i = 0; i < 1200; i++) {
		bla = malloc(sizeof(*bla));
		bla->id = rand() & 0xff;
		snprintf(bla->buf,23,"%d",bla->id);
		val = tsearch((void *) bla, &root, compare);
		if (val == NULL)
			exit(EXIT_FAILURE);
		printf("val - > 0x%08x 0x%08x\n",*(struct Bla **)val,bla);
		//else if ((*(int **) val) != ptr)
		//	free(ptr);
	}
	twalk(root, action);
	tdestroy(root, free);
	exit(EXIT_SUCCESS);
}

