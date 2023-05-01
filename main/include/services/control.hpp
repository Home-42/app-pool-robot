#ifndef SERVICE_CONTROl
#define SERVICE_CONTROl

#include "iostream"
#include "string"
#include "regex"
#include "driver/uart.h"
#include "queue"
#include "esp_log.h"
#include "esp_timer.h"

#define START_COMMAND "C+"
#define END_COMMAND ";"

#define RX_BUF_SIZE 1024
#define TX_BUF_SIZE 1024

#define MAX_FAILURE_ACTION 4
#define MAX_FAILURE_ACTION_READ 4
#define MAX_TIME_ACTION 3000000 // In microsecond

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
        bool (*check)(std::string res, void *args);
        void *args;
        int failureCount;
        int failureRead;
        int64_t startAt;
    };

    enum Moving {
        FRONT,
        BACK,
        STOP
    };

    enum Flow {
        RIGHT,
        CENTER,
        LEFT
    };

    struct Color {
        unsigned char red;
        unsigned char green;
        unsigned char blue;
    };

    struct Axes {
        float x;
        float y;
        float z;
        int64_t time;
    };

    struct ControlState {
        bool pump;
        Moving moving;
        unsigned int velocity;
        Flow flow;
        Color color;
    };

    bool checkOk(std::string res, void *args);

    class Control {
    private:
        Bridge *bridge;
        ActionState actionState;
        Action action;

        SemaphoreHandle_t actionMutex{};
        TaskHandle_t actionLoopTask{};
        [[noreturn]] [[noreturn]] void actionLoop();
        bool runAction(std::string command, bool (*check)(std::string res, void *args), void *args);

        ControlState state;
    public:
        Control(Bridge *bridge);

        void init();

        bool setPump(bool nPump);
        bool setMoving(Moving nMoving);
        bool setVelocity(unsigned int value);
        bool setFlow(Flow value);
        bool setColor(Color value);

        bool getAngle(Axes *angle);
        bool getMag(Axes *mag);
    };

}

#endif