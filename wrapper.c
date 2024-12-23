#include <stdlib.h>
#include "wrapper.h"

void *malloc_wrapper(size_t size)
{
    return malloc(size);
}
