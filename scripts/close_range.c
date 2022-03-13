#include <errno.h>

int close_range(unsigned int first, unsigned int last, unsigned int flags)
{
    errno = ENOSYS;
    return -1;
}
