#include <unistd.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_log.h"

#include "driverDHT11Interface.h"
#include "deepSleep.h"

void app_main(void) {
  deepSleepInit();
}
