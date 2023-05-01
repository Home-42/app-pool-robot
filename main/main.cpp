#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "include/services/control.hpp"

Control::Bridge bridge(UART_NUM_1);
Control::Control control(&bridge);


extern "C" void app_main(void)
{
    uart_config_t controlConfig = {
        .baud_rate = 4800,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };

//    Control::Pin controlPin = {
//            .tx = 16,
//            .rx = 17,
//            .rts = 2,
//            .cts = 2
//    };

    Control::Pin controlPin = {
        .tx = 32,
        .rx = 13,
        .rts = 2,
        .cts = 2
    };

    bridge.setConfig(&controlConfig);
    bridge.setPin(controlPin);

    bridge.init();

    control.init();


    xTaskCreate([] (void *args) {
        Control::Axes axes{};

        control.setColor({0, 255, 255});

        control.setVelocity(100);
        control.setMoving(Control::FRONT);
        vTaskDelay(500 / portTICK_PERIOD_MS);
        control.setMoving(Control::STOP);

//        control.setFlow(Control::RIGHT);
//        vTaskDelay(2000 / portTICK_PERIOD_MS);
//        control.setFlow(Control::LEFT);
//        vTaskDelay(2000 / portTICK_PERIOD_MS);
//        control.setFlow(Control::CENTER);

        while (true) {
            if (!control.getAngle(&axes)) {
                std::cout << "X:" << axes.x << ", Y: " << axes.y << ", Z: " << axes.z << std::endl;
            }
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }
    }, "wash", 10000, nullptr, 3, nullptr);
}
