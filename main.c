#include <stdlib.h>
#include "wrapper.h"

int main()
{
    void *p = malloc_wrapper(sizeof(int));
    free(p);
}
