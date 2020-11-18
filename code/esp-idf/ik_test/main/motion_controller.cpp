#include "motion_controller.h"
#include "spot_micro_motion_cmd.h"

#include "spot_micro_kinematics/utils.h"
#include "spot_micro_kinematics/spot_micro_kinematics.h"
#include "math.h"
using namespace smk;

#include <esp_log.h>
static char tag[] = "IKTEST";



// Spot Micro Kinematics object. Holds kinematic state of robot, and holds
// kinematics operations for setting position/orientation of the robot
// smk::SpotMicroKinematics sm_; 

/**
 * Publish configuration to the motor driver
 */
static int publishServoConfiguration()
{
    //TODO: set_servo(uint8_t id, uint16_t angle)
    return 0;
}


extern "C" int controller_init()
{
    // initialize state machine
    SpotMicroMotionCmd node();

    //TODO: servo angles state configuration


    if(publishServoConfiguration() != 0)
        return -1;

    return 0;
}


void controller_command_add()
{

}

/**
 * Control Loop at fixed rate of frames/second refresh cycle for servos.
 */
int controller_run()
{
    //TODO: read all received commands

    //TODO: process and clear commands

    return 0;
}

/**
 * Return robot status
 */
int controller_status()
{
    //TODO: return robot status
    return 0;
}