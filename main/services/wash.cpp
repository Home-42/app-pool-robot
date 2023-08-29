#include "../include/services/wash.hpp"

int randInt(int from, int to) {
    float lambda = (float)esp_random() / 4294967296;
    float res = lambda * from + (1 - lambda) * to;
    return (int) res;
}

Wash::Wash::Wash(Control::Control *control) {
    this->control = control;
}

Wash::AngleDelta Wash::Wash::waitWallRecursive(float wallAngle, int skipTime) {
    AngleDelta angleDelta{};
    Control::Axes previousAngles{};
    Control::Axes currentAngles{};
    int64_t previousTime = esp_timer_get_time() / 1000;
    int64_t currentTime;

    control->getAngle(&previousAngles);

    do {
        control->getAngle(&currentAngles);
        currentTime = esp_timer_get_time() / 1000 - previousTime;
        angleDelta.angle = previousAngles.x - currentAngles.x;
        angleDelta.skipTime = currentTime - previousTime >= skipTime;
    } while (abs(angleDelta.angle) < wallAngle && !angleDelta.skipTime);

    return angleDelta;
}

Wash::AngleDelta Wash::Wash::turn(Control::Flow flow, int minAngle, int skipTime) {
    AngleDelta angleDelta{};
    Control::Axes previousAngles{};
    Control::Axes currentAngles{};
    int64_t previousTime = esp_timer_get_time() / 1000;
    int64_t currentTime;

    control->getAngle(&previousAngles);
    control->setVelocity(100);

    do {
        control->setPump(false);
        control->setMoving(Control::BACK);
        control->setFlow(flow);
        vTaskDelay(2500 / portTICK_PERIOD_MS);
        control->setMoving(Control::FRONT);
        vTaskDelay(3500 / portTICK_PERIOD_MS);

        control->setPump(true);
        vTaskDelay(500 / portTICK_PERIOD_MS);
        control->setMoving(Control::STOP);
        vTaskDelay(2500 / portTICK_PERIOD_MS);

        control->setFlow(Control::CENTER);
        vTaskDelay(500 / portTICK_PERIOD_MS);

        control->getAngle(&currentAngles);
        currentTime = esp_timer_get_time() / 1000 - previousTime;

        angleDelta.angle = previousAngles.z - currentAngles.z;
        angleDelta.skipTime = currentTime - previousTime >= skipTime;
        angleDelta.skipTime = currentTime - previousTime >= skipTime;

    } while (abs(angleDelta.angle) < (float)minAngle && !angleDelta.skipTime);

    return angleDelta;
}


[[noreturn]] void Wash::Wash::loopFloor() {
    int64_t startTime = esp_timer_get_time();
    int64_t currentTime;
    control->setColor({0, 255, 255});
    long int turn = 0;

    do {
        control->setVelocity(100);
        control->setPump(true);
        control->setMoving(Control::FRONT);
        this->waitWallRecursive(WALL_ANGLE, SKIP_ACTION_TIME);
        control->setMoving(Control::STOP);
        control->setMoving(Control::BACK);
        int backTime = randInt(BACK_RANDOM_FROM, BACK_RANDOM_TO);
        vTaskDelay(backTime / portTICK_PERIOD_MS);

        Control::Flow flow = turn % 4 <= 1 ? Control::RIGHT : Control::LEFT;

        int turnAngle = randInt(TURN_ANGLE_MIN, TURN_ANGLE_MAX);

        if (this->turn(flow, turnAngle, SKIP_ACTION_TIME).skipTime) {
            this->turn(Control::oppositeFlow(flow), turnAngle, SKIP_ACTION_TIME);
            turn--;
        }

        turn++;

        currentTime = esp_timer_get_time();
        std::cout << (currentTime - startTime) / (WASH_TIME * 1000) << "%" <<  std::endl;
    } while  (currentTime - startTime < WASH_TIME * 1000);

    control->setPump(false);
    control->setMoving(Control::STOP);

    while (true) {
        control->setColor({0, 255, 0});
        vTaskDelay(500 / portTICK_PERIOD_MS);
        control->setColor({0, 0, 0});
        vTaskDelay(600 / portTICK_PERIOD_MS);
    }
}


void Wash::Wash::startFloor() {
    xTaskCreate([](void *args) {
        auto *wash = (Wash *)args;
        wash->loopFloor();
    }, "wash floor", 10000, this, 2, &this->washTask);
}


void Wash::Wash::stop() {
    if (this->washTask != nullptr) {
        vTaskDelete(this->washTask);
        this->washTask = nullptr;
    }
}