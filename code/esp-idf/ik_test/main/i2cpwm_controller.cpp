
#include "i2cpwm_controller.h"

namespace i2cpwm_board
{

int i2cpwm_board_init()
{
    //TODO:
    return 0;
}

int i2cpwm_board_config(std::map<int, ServoConfig> servo_config)
{
    //TODO: cache the config
    return 0;
}

void servos_absolute (const ServoArray servos)
{
    //TODO: command PCA9685
}

void servos_proportional (const ServoArray servos)
{
    //TODO: command PCA9685
}
}