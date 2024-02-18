

#include "esp_log.h"

#include "driverDHT11Interface.h"

#if defined(CONFIG_MYAPP_WAKEUP_EN)
  #include "deepSleep.h"
#endif

const static char *MAIN = "esp_apptrace_uart";

void app_main(void) {

  uint8_t ok = true;

#if defined(CONFIG_MYAPP_WAKEUP_EN)
  ok &= deepSleepInit();
#else
#warning "Deep sleep not used in the application"
  ESP_LOGI(MAIN, "Deep sleep not used in the application");
#endif

  ok &= dht11_interface_init();

}
