#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "include/services/control.hpp"

Control::Bridge bridge(UART_NUM_1);
Control::Control control(&bridge);

void washTask(void *args) {
    vTaskDelay(3000 / portTICK_PERIOD_MS);
    std::cout << "RUNNING TASK" << std::endl;
    bool error = control.runAction("GO@FRONT\n", [] (std::string res) {
        return res == "OK\n";
    });
    std::cout << "ERROR: " << error << std::endl;

    while (true) {
        vTaskDelay(3000 / portTICK_PERIOD_MS);
    }
}

extern "C" void app_main(void)
{
    uart_config_t controlConfig = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };

    Control::Pin controlPin = {
        .tx = 16,
        .rx = 17,
        .rts = 2,
        .cts = 2
    };

    bridge.setConfig(&controlConfig);
    bridge.setPin(controlPin);

    bridge.init();

    control.init();

    xTaskCreate(washTask, "wash", 4096, nullptr, 3, nullptr);
}
