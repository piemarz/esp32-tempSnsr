#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>

#include "esp_sleep.h"
#include "esp_timer.h"

#include "driver_dht11.h"

void app_main(void)
{
    while (true) {
        printf("Hello from app_main!\n");
        sleep(1);
    }
}
