#include "app/app_main.h"
#include "systick.h"

int main(void)
{
    systick_config();
    app_main_init();
    app_main_run();
    return 0;
}

