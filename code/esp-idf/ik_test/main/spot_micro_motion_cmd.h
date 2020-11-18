
#pragma once //designed to include the current source file only once in a single compilation.
#ifndef SPOT_MICRO_MOTION_CMD //usd for conditional compiling.
#define SPOT_MICRO_MOTION_CMD

// #include "command.h"
// #include "spot_micro_kinematics.h"
// #include "spot_micro_state.h"

class SpotMicroMotionCmd
{
    public:
        SpotMicroMotionCmd(); //constructor method
        ~SpotMicroMotionCmd(); // distructor method
        void runOnce(); // runOnce method to control the flow of program

        // Publish a servo configuration message
        bool publishServoConfiguration();

    private:
        // Declare SpotMicroState a friend so it can access and modify private
        // members of this class
        // friend class SpotMicroState;

        // Spot Micro Kinematics object. Holds kinematic state of robot, and holds
        // kinematics operations for setting position/orientation of the robot
        // smk::SpotMicroKinematics sm_; 
};
#endif  