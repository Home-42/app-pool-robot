#include "include/main.hpp"

Control::Bridge bridge(UART_NUM_1);
Control::Control control(&bridge);
Wash::Wash wash(&control);

extern "C" void app_main(void)
{
    uart_config_t controlConfig = {
        .baud_rate = BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };

    Control::Pin controlPin = {
        .tx = PIN_TX,
        .rx = PIN_RX,
        .rts = PIN_RTS,
        .cts = PIN_CTS
    };


    bridge.setConfig(&controlConfig);
    bridge.setPin(controlPin);

    vTaskDelay(5000 / portTICK_PERIOD_MS);

    bridge.init();
    control.init();
    wash.startFloor();
}
