#include <utility>

#include "../include/services/control.hpp"


[[noreturn]] void Control::Bridge::loopRead() {
    char* data = (char*) malloc(TX_BUF_SIZE + 1);
    while (true) {
        if (this->readMutex != nullptr) {
            xSemaphoreTakeRecursive(this->readMutex, 5);
            const int rxBytes = uart_read_bytes(this->uartNum, data, TX_BUF_SIZE, 250 / portTICK_RATE_MS);
            if (rxBytes > 0) {
                data[rxBytes] = 0;
                std::string strData;
                strData.assign(data, rxBytes);
                this->readQueue.push(strData);
//                ESP_LOGI("CONTROL", "Read %d bytes: '%s'", rxBytes, data);
            }
            xSemaphoreGiveRecursive(this->readMutex);
        }
        vTaskDelay(5 / portTICK_PERIOD_MS);
    }
}


Control::Bridge::Bridge(int uartNum) {
    this->uartNum = uartNum;
}

void Control::Bridge::setConfig(uart_config_t *config) {
    this->config = config;
}

void Control::Bridge::setPin(Pin pin) {
    this->pin = pin;
}

void Control::Bridge::init() {
    ESP_ERROR_CHECK(uart_param_config(this->uartNum, this->config));
    ESP_ERROR_CHECK(uart_set_pin(this->uartNum, this->pin.tx, this->pin.rx, this->pin.rts, this->pin.cts));
    ESP_ERROR_CHECK(uart_driver_install(this->uartNum, RX_BUF_SIZE, TX_BUF_SIZE, 10, &this->uartQueue, 0));
    this->readMutex = xSemaphoreCreateRecursiveMutex();
    auto callback = [] (void *args) {
        auto * control = (Bridge *)args;
        control->loopRead();
    };
    xTaskCreate(callback, "bridge read", 10000, this, 1, &this->readTask);
}

void Control::Bridge::send(std::string msg) const {
    uart_write_bytes(this->uartNum, msg.c_str(), msg.size());
}


bool Control::Bridge::available() {
    xSemaphoreTakeRecursive(this->readMutex, 5);
    bool available = !this->readQueue.empty();
    xSemaphoreGiveRecursive(this->readMutex);
    return available;
}

std::string Control::Bridge::read() {
    xSemaphoreTakeRecursive(this->readMutex, 5);
    std::string res;
    if (!this->readQueue.empty()) {
        res = this->readQueue.front();
        this->readQueue.pop();
    }
    xSemaphoreGiveRecursive(this->readMutex);
    return res;
}

void Control::Bridge::clearRead() {
    xSemaphoreTakeRecursive(this->readMutex, 5);
    while (!this->readQueue.empty()) {
        this->readQueue.pop();
    }
    xSemaphoreGiveRecursive(this->readMutex);
}

bool Control::checkOk(std::string res, void *args) {
    return res.find("OK") != std::string::npos;
}


[[noreturn]] void Control::Control::actionLoop() {
    ActionState oldState = this->actionState;
    std::string data;
    int64_t currentTime;
    std::string res;

    while(true) {
        if (this->actionMutex != nullptr) {
            xSemaphoreTakeRecursive(this->actionMutex, 5);
            if (this->actionState == LOADED) {
                if (this->actionState != oldState) {
//                    std::cout << "LOADED" << std::endl;
                    oldState = this->actionState;
                }
                data = START_COMMAND + this->action.command + END_COMMAND;
                this->bridge->send(data);
                this->actionState = PENDING;
            } else if (this->actionState == PENDING) {
                if (this->actionState != oldState) {
//                    std::cout << "PENDING" << std::endl;
                    oldState = this->actionState;
                }
                currentTime = esp_timer_get_time();
                if (currentTime - this->action.startAt > MAX_TIME_ACTION) {
                    this->actionState = ERROR;
                } else {
                    if (this->bridge->available()) {
//                        std::cout << "bridge is available" << std::endl;
                        res = this->bridge->read();
                        if (this->action.check(res, this->action.args)) {
                            this->actionState = DONE;
                        } else {
                            if (this->action.failureCount >= MAX_FAILURE_ACTION) {
                                this->actionState = ERROR;
                            } else if (this->action.failureRead >= MAX_FAILURE_ACTION_READ){
                                this->actionState = LOADED;
                                this->action.failureCount ++;
                                this->action.failureRead = 0;
                            } else {
                                this->action.failureRead ++;
                            }
                        }
                    }
                }
            }
            else if (this->actionState == ERROR) {
                if (this->actionState != oldState) {
//                    std::cout << "ERROR" << std::endl;
                    oldState = this->actionState;
                }
            }
            else if (this->actionState == IDLE) {
                if (this->actionState != oldState) {
//                    std::cout << "IDLE" << std::endl;
                    oldState = this->actionState;
                }
            }
            else if (this->actionState == DONE) {
                if (this->actionState != oldState) {
//                    std::cout << "DONE" << std::endl;
                    oldState = this->actionState;
                }
            }
            xSemaphoreGiveRecursive(this->actionMutex);
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

Control::Control::Control(Bridge *bridge) {
    this->bridge = bridge;
    this->actionState = IDLE;
    this->state = {false, STOP, 0, CENTER, {0, 0, 0}};
}

void Control::Control::init() {
    this->actionMutex = xSemaphoreCreateRecursiveMutex();

    auto callback = [] (void *args) {
        auto *control = (Control *)args;
        control->actionLoop();
    };
    xTaskCreate(callback, "control action", 20000, this, 2, &this->actionLoopTask);
}

bool Control::Control::runAction(std::string command, bool (*check)(std::string res, void *args), void *args) {
    this->bridge->clearRead();
    xSemaphoreTakeRecursive(this->actionMutex, 5);
    this->actionState = LOADED;
    this->action.failureCount = 0;
    this->action.failureRead = 0;
    this->action.command = std::move(command);
    this->action.check = check;
    this->action.args = args;
    this->action.startAt = esp_timer_get_time();
    xSemaphoreGiveRecursive(this->actionMutex);

    while(this->actionState != DONE && this->actionState != ERROR) {
        vTaskDelay(15 / portTICK_PERIOD_MS);
    }
    bool error = this->actionState == ERROR;

    this->actionState = IDLE;

    return error;
}



bool Control::Control::setPump(bool nPump) {
    if (this->state.pump == nPump) {
        return true;
    }
    this->state.pump = nPump;
    return this->runAction("S_P:" + std::string(nPump ? "1" : "0"), checkOk, nullptr);
}

bool Control::Control::setMoving(Moving nMoving) {
    if (this->state.moving == nMoving) {
        return true;
    }
    this->state.moving = nMoving;
    std::string direction;
    switch (nMoving) {
        case Moving::BACK: direction = 'B'; break;
        case Moving::FRONT: direction = 'F'; break;
        case Moving::STOP: direction = 'S'; break;
    }
    return this->runAction("S_RM:" + direction, checkOk, nullptr);
}

bool Control::Control::setVelocity(unsigned int value) {
    if (this->state.velocity == value) {
        return true;
    }
    this->state.velocity = value;
    return this->runAction("S_RV:" + std::to_string(value), checkOk,nullptr);
}

bool Control::Control::setFlow(Flow value) {
    if (this->state.flow == value) {
        return true;
    }
    this->state.flow = value;
    std::string direction;
    switch (value) {
        case RIGHT: direction = 'R'; break;
        case CENTER: direction = 'C'; break;
        case LEFT: direction = 'L'; break;
    }
    return this->runAction("S_F:" + direction, checkOk, nullptr);
}

bool Control::Control::setColor(Color value) {
    Color oldColor = this->state.color;
    if (oldColor.red == value.red && oldColor.green == value.green && oldColor.blue == value.blue ) {
        return true;
    }
    this->state.color = value;
    std::string color;
    color += std::to_string(value.red) + ",";
    color += std::to_string(value.green) + ',';
    color += std::to_string(value.blue);
    return this->runAction("S_CRGB:" + color, checkOk, nullptr);
}

bool Control::Control::getAngle(Axes *angle) {
    auto check = [] (std::string res, void *args) {
        auto *a = (Axes *)args;
        std::regex rgx(R"(A:((\d|\.|-)*),((\d|\.|-)*),((\d|\.|-)*))");
        std::smatch match;
        if (std::regex_search(res, match, rgx)) {
            a->z = std::stof(match[1]); // z
            a->x = std::stof(match[3]); // x
            a->y = std::stof(match[5]); // y
            a->time = esp_timer_get_time();
            return true;
        } return false;
    };
    return this->runAction("G_A", check, angle);
}

bool Control::Control::getMag(Axes *mag) {
    auto check = [] (std::string res, void *args) {
        auto *a = (Axes *)args;
        std::regex rgx(R"(M:((\d|\.|-)*),((\d|\.|-)*),((\d|\.|-)*))");
        std::smatch match;
        if (std::regex_search(res, match, rgx)) {
            a->z = std::stof(match[1]); // z
            a->x = std::stof(match[3]); // x
            a->y = std::stof(match[5]); // y
            a->time = esp_timer_get_time();
            return true;
        } return false;
    };
    return this->runAction("G_M", check, mag);
}
