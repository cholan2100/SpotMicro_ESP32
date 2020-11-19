
#include "i2cpwm_controller.h"
extern "C" {
#include "pca9685.h"
#include "i2c_app.h"
#include "config.h"
}

namespace i2cpwm_board
{

/**
 * @brief pac 9685 initialization
 */
static void init_pca9685() {
    i2c_example_master_init();
    set_pca9685_adress(I2C_ADDRESS);
    resetPCA9685();
    setFrequencyPCA9685(50);
}

int i2cpwm_board_init()
{
    init_pca9685();
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