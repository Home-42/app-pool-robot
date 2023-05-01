#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

#include "include/services/control.hpp"
#include "include/services/wash.hpp"

#define BAUD_RATE 4800
#define PIN_TX 32
#define PIN_RX 13
#define PIN_RTS 2
#define PIN_CTS 2
