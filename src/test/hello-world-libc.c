#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static const char *hello_world = "‶Hello World″\n";

int fake[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

int main()
{
    fake[0] = 0x123;
    fake[1] = fake[0];
    if (write(0, hello_world, strlen(hello_world)) != strlen(hello_world)) exit(9);
    return 0;
}
