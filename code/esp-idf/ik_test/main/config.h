#ifndef CONFIG_H
#define CONFIG_H

#include "servo.h"

// SERVOS
#define Servo_Shoulder 0
#define Servo_Leg      1
#define Servo_Foot     2

#define LEG_LF  0 // 0, 1, 2
#define LEG_RF  1 // 3, 4, 5
#define LEG_LB  2 // 6, 7, 8
#define LEG_RB  3 // 9, 10, 11

#define L1  60.5  // y Distance between Shoulder Servo and Leg
#define L2  10    // z Distance between Shoulder Servo and Leg
#define L3  111.1 // Length of upper leg //FIXME: was 100.7
#define L4  118.5 // Length of lower leg
#define L   207.5 // Distance between front and back servos
#define W   78    // Distance between left and right shoulder

// predefined calculations
#define L1L1  3660.25  //L1*L1
// #define L1L2  3760.25  //L1*L1+L2*L2
// #define LL12  1210     //2*L1*L2
#define L3L3  10140.49 //L3*L3
#define L4L4  14042.25 //L4*L4
#define LL34  23865.9  //2*L3*L4

#define SERVO_STEP_ANGLE 2
#define MOTION_STEP_ANGLE 5
#define MOTION_STEP_MOVEMENT 5
#define MOTION_STEP_ALFA 0.20

extern const int16_t servo_min[12] ;
extern float servo_conversion[12] ;
extern const float theta_range[3][2];
extern const int8_t servo_invert[12];
extern int16_t servo_angles[4][3];



#define I2C_MASTER_FREQ_HZ  100000     /*!< I2C master clock frequency */
#define I2C_MASTER_NUM      I2C_NUM_0   /*!< I2C port number for master dev */
#define I2C_MASTER_TX_BUF_DISABLE   0   /*!< I2C master do not need buffer */
#define I2C_MASTER_RX_BUF_DISABLE   0   /*!< I2C master do not need buffer */


#define ACK_CHECK_EN    0x1     /*!< I2C master will check ack from slave */
#define ACK_CHECK_DIS   0x0     /*!< I2C master will not check ack from slave */
#define ACK_VAL         0x0     /*!< I2C ack value */
#define NACK_VAL        0x1     /*!< I2C nack value */

/// Configure for Spot Micro Hardware on ESP32
#define I2C_MASTER_SCL_IO   22    /*!< gpio number for I2C master clock */
#define I2C_MASTER_SDA_IO   23    /*!< gpio number for I2C master data  */
#define I2C_ADDRESS         0x41    /*!< slave address for PCA9685, DEFAULT: 0x40 */
#define SERVO_PWM_MIN       {90,81,90, 95,94,75, 86,100,96, 115,110,85} //FLS,FLU,FLL, FRS,FRU,FRL, RLS,RLU,RLL, RRS,RRU,RRL
//FIXME: max values are guessed from first servo, instead find the actual duty cycleint8_t pwm_channel[12] = 
#define SERVO_PWM_MAX       {510,501,510, 515,514,495, 506,520,516, 525,530,505} //FLS,FLU,FLL, FRS,FRU,FRL, RLS,RLU,RLL, RRS,RRU,RRL

#define SERVO_CHANNELS      {/* FLS,FLU,FLL */ 12,13,14, \
                             /* FRS,FRU,FRL */   8,9,10, \
                             /* RLS,RLU,RLL */   4,5,6, \
                             /* RRS,RRU,RRL */   0,1,2 } 
#endif