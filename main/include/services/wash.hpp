#ifndef SERVICE_WASH
#define SERVICE_WASH

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"

#include "./control.hpp"

#define WASH_TIME (int64_t)1*3600*1000
#define BACK_RANDOM_FROM 10000
#define BACK_RANDOM_TO 20000
#define SKIP_ACTION_TIME 400000
#define WALL_ANGLE 20
#define TURN_ANGLE 90

namespace Wash {
    struct AngleDelta {
        float angle;
        bool skipTime;
    };

    class Wash {
    private:
        Control::Control *control;
        TaskHandle_t washTask;

        [[noreturn]] void loopFloor();
    public:
        Wash(Control::Control *control);

        AngleDelta turn(Control::Flow flow, int minAngle, int skipTime);
        AngleDelta waitWallRecursive(float wallAngle, int skipTime);

        void startFloor();

        void stop();
    };
}

#endif