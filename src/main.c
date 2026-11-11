#include "platform.h"

#ifndef CONVOY_SMOKE
#define CONVOY_SMOKE 0
#endif

int main(void)
{
    plat_init();
#if CONVOY_SMOKE
    plat_smoke();
    return 0;
#else
    for (;;)
        plat_frame();
#endif
}
