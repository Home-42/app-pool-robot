#ifndef SERVICE_CONTROl
#define SERVICE_CONTROl

#include "iostream"
#include "string"
#include "driver/uart.h"
#include "queue"
#include "esp_log.h"
#include "esp_timer.h"

#define RX_BUF_SIZE 1024
#define TX_BUF_SIZE 1024

#define MAX_FAILURE_ACTION 5
#define MAX_TIME_ACTION 6000000 // In microsecond

namespace Control {
    struct Pin {
        int tx;
        int rx;
        int rts;
        int cts;
    };


    class Bridge {
    private:
        uart_config_t *config;
        Pin pin;
        int uartNum;

        QueueHandle_t uartQueue;

        TaskHandle_t readTask;
        SemaphoreHandle_t readMutex;
        std::queue<std::string> readQueue;

        [[noreturn]] void loopRead();
    public:
        Bridge(int uartNum);

        void setConfig(uart_config_t *config);
        void setPin(Pin pin);

        void init();

        void send(std::string msg) const;
        bool available();
        std::string read();
        void clearRead();
    };


    enum ActionState {
        IDLE,
        LOADED,
        PENDING,
        ERROR,
        DONE
    };

    struct Action {
        std::string command;
        bool (*check)(std::string res);
        int failureCount;
        int64_t startAt;
    };

    class Control {
    private:
        Bridge *bridge;
        ActionState actionState;
        Action action;

        SemaphoreHandle_t actionMutex{};
        TaskHandle_t actionLoopTask{};
        [[noreturn]] [[noreturn]] void actionLoop();

    public:
        Control(Bridge *bridge);

        bool runAction(std::string command, bool (*check)(std::string res));
        void init();
    };

}

#endif