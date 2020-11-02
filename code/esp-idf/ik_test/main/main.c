/*
Test IK implementation
Author Maarten Weyn
*/

#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <math.h>

#include "nvs.h"
#include "nvs_flash.h"


#include "esp_dsp.h"

#include "pca9685.h"
#include "i2c_app.h"
#include "sdkconfig.h"
#include "spot_ik.h"

#include "servo.h"
#include "config.h"
#include "bt_spp.h"

static char tag[] = "IKTEST";

#define DEGREES2RAD 0.017453292519943

typedef  struct {
    float omega;
    float phi;
    float psi;
    float xm;
    float ym;
    float zm;
    bool set;
} position_t;

// Rest position to begin with
position_t spot_position = {.omega=0,.phi=0,.psi=0,.xm=-40,.ym=-170, .zm=0, .set=1};
position_t goal_position = {0,};

const int16_t servo_min[12] = SERVO_PWM_MIN;
const int16_t servo_max[12] = SERVO_PWM_MAX;

float servo_conversion[12];
                                //LF     //RF   //LB    //RB
const int8_t servo_invert[12] = {1,0,1, 0,1,0,  0,0,1,  1,1,0};
const float theta_range[3][2] = {{-M_PI / 3, M_PI/3}, {-2 * M_PI/3, M_PI/3}, {0, M_PI}};

#undef ESP_ERROR_CHECK
#define ESP_ERROR_CHECK(x)   do { esp_err_t rc = (x); if (rc != ESP_OK) { ESP_LOGE("err", "esp_err_t = %d", rc); assert(0 && #x);} } while(0);

/**
 * @brief pac 9685 initialization
 */
void init_pca9685() {
    i2c_example_master_init();
    set_pca9685_adress(I2C_ADDRESS);
    resetPCA9685();
    setFrequencyPCA9685(50);
}

float omega = 0; //Rx
float psi = 0;  // Rz
float height = 200;

//note: rest angles be default
int16_t servo_angles[4][3] = {{0,0,0},{0,0,0},{0,0,0},{0,0,0}};//{{90, 150, 0}, {90, 30, 180}, {90, 150, 0}, {90, 30, 180}}; // down angle
int16_t servo_angles_goal[4][3] = {0,};
float p[4][3] = {{-L1, -L3-L2, L4},{-L1, -L3-L2, L4},{-L1, -L3-L2, L4},{-L1, -L3-L2, L4}};

#define RAD2DEGREES 57.295779513082321 // 180 / PI


void set_orientation_cb(int16_t omega, int16_t phi, int16_t psi, int16_t xm, int16_t ym, int16_t zm);
void test_servo() {
    esp_err_t ret = 0;

    int16_t start = 60;
    int16_t end = 120;
    int16_t step = 5;
    int16_t current = (start+end) / 2;
    while(1)
    {
        vTaskDelay(500 / portTICK_RATE_MS);

        //ret = set_servo(2, current);
        //ret = set_servo(8, 180-current);

        if(ret == ESP_FAIL) ESP_LOGD(tag, "set servo error");

        current += step;
        if (current > end) {
            step *= -1;
            current+= 2 * step;
        } else if (current < start) {
            step *= -1;
            current+= 2 * step;
        }
    }
}

void calculate_foot_points() {
float tan_omega = tan(omega);
    float tan_psi = tan(psi);
    // Front has impact of omega
    
    float h_offset = (W/2.0 + L1) * tan_omega;

    //Front Left Leg
    p[0][1]= - (height - h_offset);
    p[0][0] = -L1 + p[0][1]* tan_omega ;
    p[0][2] = p[0][1]* tan_psi;

    // Front Right leg
    p[1][1]= - (height + h_offset);
    p[1][0] = L1 - p[1][1]* tan_omega ;
    p[1][2] = p[1][1]* tan_psi;

    // Rear correct based on psi
    float height_rear = height + L * tan_psi;
    //Front Left Leg
    p[2][1]= - (height_rear - h_offset);
    p[2][0] = -L1 + p[2][1]* tan_omega;
    p[2][2] = p[2][1]* tan_psi;

    // Front Right leg
    p[3][1]= - (height_rear + h_offset);
    p[3][0] = L1 - p[3][1]* tan_omega ;
    p[3][2] = p[3][1]* tan(psi);

    ESP_LOGI(tag, "calculate_foot_points ( onega %d, psi %d, height %.1f mm):", (int) (RAD2DEGREES * omega), (int) (RAD2DEGREES * psi),  height);
    ESP_LOGI(tag, "Front Left Leg (x,z,y) (%.1f,%.1f,%.1f)",  p[0][0], p[0][2], p[0][1]);
    ESP_LOGI(tag, "Front Right Leg (x,z,y) (%.1f,%.1f,%.1f)", p[1][0], p[1][2], p[1][1]);
    ESP_LOGI(tag, "Rear Left Leg (x,z,y) (%.1f,%.1f,%.1f)",   p[2][0], p[2][2], p[2][1]);
    ESP_LOGI(tag, "Rear Right Leg (x,z,y) (%.1f,%.1f,%.1f)",  p[3][0], p[3][2], p[3][1]);
}

esp_err_t calculate_leg_positions() {
    for (int l = 0; l<4; l++) {
        esp_err_t ret = leg_IK(p[l], l, servo_angles[l]);
        ESP_LOGI(tag, "IK (x,z,y) (%.1f, %.1f, %.1f) -> (%d, %d, %d) (%d)", p[l][0], p[l][2], p[l][1], servo_angles[l][0], servo_angles[l][1], servo_angles[l][2], ret);
        if (ret != ESP_OK) return ESP_FAIL;
    }

    return ESP_OK;
}

void set_leg_servos_in_steps() {
    bool goal_reached = false;
    do {
        goal_reached = true;
        for (int l = 0; l<4; l++) {
        // for (int l = 1; l<2; l++) {    
            for (int s=0;s<3;s++) {
                int diff = servo_angles[l][s] - servo_angles_goal[l][s];
                if (abs(diff) < SERVO_STEP_ANGLE) {
                    servo_angles[l][s] = servo_angles_goal[l][s];
                } else {
                    goal_reached = false;
                    diff = diff < 0 ?  SERVO_STEP_ANGLE : -SERVO_STEP_ANGLE;
                    servo_angles[l][s] += diff;
                }
                if (diff != 0)
                    set_servo(l*3 + s, servo_angles[l][s]);
            }
        }

        if (!goal_reached) {
            print_int_matrix((int16_t*) servo_angles, 4, 3, "servo_angles", false);
            //vTaskDelay(10 / portTICK_RATE_MS);
        }

    } while (!goal_reached) ;
}

void set_leg_servos() {
    for (int l = 0; l<4; l++) {
        for (int s=0;s<3;s++) {
            // if (servo_angles[l][s] != servo_angles_goal[l][s]) 
            {
                servo_angles[l][s] = servo_angles_goal[l][s];
                set_servo(l*3 + s, servo_angles[l][s]);
            }
        }
    }
}

inline void set_foot_position(uint8_t leg_id, float x, float z, float y) {
    p[leg_id][0] = x;
    p[leg_id][2] = z;
    p[leg_id][1]= y;
}

void sleep_position()
{
    set_foot_position(LEG_LF, -L1, L4-L3, -L2);
    set_foot_position(LEG_RF, L1, L4-L3, -L2);  
    set_foot_position(LEG_LB, -L1, L4-L3, -L2);  
    set_foot_position(LEG_RB, L1, L4-L3, -L2);  
    esp_err_t ret = calculate_leg_positions();
    if (ret == ESP_OK) set_leg_servos();
}

void reset_position() {
    set_orientation_cb(0, 0, 0, -40, -170, 0);  // sleep
}

esp_err_t set_legs() {
    calculate_foot_points();
    esp_err_t ret = calculate_leg_positions();
    if (ret == ESP_OK) set_leg_servos();

    return ret;
}

static int spot_state = 0;
#define SPOT_STATE_SLEEP    0
#define SPOT_STATE_WAKE     1

void set_orientation_cb(int16_t omega, int16_t phi, int16_t psi, int16_t xm, int16_t ym, int16_t zm)
{
    spot_state = SPOT_STATE_WAKE;   
    if(xm == -40 && ym == -170 && zm == 0)  // from python siimulations
        spot_state = SPOT_STATE_SLEEP;
    
    goal_position.omega = omega;
    goal_position.phi = phi;
    goal_position.psi = psi;
    goal_position.xm  = xm;
    goal_position.ym = ym;
    goal_position.zm = zm;
    goal_position.set = true;
}

static void spot_sleep()
{
    ESP_LOGI(tag, "Hard Sleep - manual servo override");
    int angle[3] = {90,30,0};
    for(int s = 0; s<3; s++)
    {
        for (int l = 0; l<4; l++)
        {
            //servo_invert[12] = {1,0,1, 0,1,0,  0,0,1,  1,1,0};
            if (servo_invert[(l*3)+s] == 0)
                servo_angles_goal[l][s] = 180 - angle[s];
            else
                servo_angles_goal[l][s] = 0 + angle[s];
            
        }
    }


    spot_position.omega = spot_position.phi = spot_position.psi = 0;
    spot_position.xm = -40;
    spot_position.ym = -170;
    spot_position.zm = 0;
}

void iterate_to_position() {
    ESP_LOGI(tag, "GOAL (%f,%f,%f - %f,%f,%f)", goal_position.omega, goal_position.phi, goal_position.psi, goal_position.xm, goal_position.ym, goal_position.zm);

    do {
    spot_position.set = false;
    int diff = 0;

    if (goal_position.omega != spot_position.omega) {
        diff = goal_position.omega - spot_position.omega;
        if (abs(diff) < MOTION_STEP_ANGLE) {
            spot_position.omega = goal_position.omega ;
        } else {
            diff = diff < 0 ?  -MOTION_STEP_ANGLE : MOTION_STEP_ANGLE;
            spot_position.omega += diff;
            spot_position.set = true;
        }
    }

    if (goal_position.phi != spot_position.phi) {
        diff = goal_position.phi - spot_position.phi;
        if (abs(diff) < MOTION_STEP_ANGLE) {
            spot_position.phi = goal_position.phi ;
        } else {
            diff = diff < 0 ?  -MOTION_STEP_ANGLE : MOTION_STEP_ANGLE;
            spot_position.phi += diff;
            spot_position.set = true;
        }
    }

    if (goal_position.psi != spot_position.psi) {
        diff = goal_position.psi - spot_position.psi;
        if (abs(diff) < MOTION_STEP_ANGLE) {
            spot_position.psi = goal_position.psi ;
        } else {
            diff = diff < 0 ?  -MOTION_STEP_ANGLE : MOTION_STEP_ANGLE;
            spot_position.psi += diff;
            spot_position.set = true;
        }
    }

    if (goal_position.xm != spot_position.xm) {
        diff = goal_position.xm - spot_position.xm;
        if (abs(diff) < MOTION_STEP_MOVEMENT) {
            spot_position.xm = goal_position.xm ;
        } else {
            diff = diff < 0 ?  -MOTION_STEP_MOVEMENT : MOTION_STEP_MOVEMENT;
            spot_position.xm += diff;
            spot_position.set = true;
        }
    }

    if (goal_position.ym != spot_position.ym) {
        diff = goal_position.ym - spot_position.ym;
        if (abs(diff) < MOTION_STEP_MOVEMENT) {
            spot_position.ym = goal_position.ym ;
        } else {
            diff = diff < 0 ?  -MOTION_STEP_MOVEMENT : MOTION_STEP_MOVEMENT;
            spot_position.ym += diff;
            spot_position.set = true;
        }
    }

    if (goal_position.zm != spot_position.zm) {
        diff = goal_position.zm - spot_position.zm;
        if (abs(diff) < MOTION_STEP_MOVEMENT) {
            spot_position.zm = goal_position.zm ;
        } else {
            diff = diff < 0 ?  -MOTION_STEP_MOVEMENT : MOTION_STEP_MOVEMENT;
            spot_position.zm += diff;
            spot_position.set = true;
        }
    }


    ESP_LOGI(tag, "CURRENT (%f,%f,%f - %f,%f,%f) %d", spot_position.omega, spot_position.phi, spot_position.psi, spot_position.xm, spot_position.ym, spot_position.zm, spot_position.set);
    
    esp_err_t ret = spot_IK(spot_position.omega*DEGREES2RAD, spot_position.phi*DEGREES2RAD, spot_position.psi*DEGREES2RAD, 
                            spot_position.xm, spot_position.ym, spot_position.zm, 
                            servo_angles_goal);
    ESP_LOGD(tag, "Valid IK %d", ret==ESP_OK);
    if (ret == ESP_OK) {
        print_int_matrix((int16_t*) servo_angles_goal, 4, 3, "servo_angles_goal", false);
    }

    } while (spot_position.set);
}

#if 0
void iterate_to_position_exponetial() {
    ESP_LOGI(tag, "GOAL (%f,%f,%f - %f,%f,%f)", goal_position.omega, goal_position.phi, goal_position.psi, goal_position.xm, goal_position.ym, goal_position.zm);

    do {
        spot_position.set = false;
        // int diff = 0;

        if (abs(goal_position.omega - spot_position.omega) < MOTION_STEP_ANGLE) {
            spot_position.omega = goal_position.omega;
        } else {
            spot_position.set = true;
            spot_position.omega = MOTION_STEP_ALFA * goal_position.omega + (1.0 - MOTION_STEP_ALFA)  *spot_position.omega;
        }

        if (abs(goal_position.phi - spot_position.phi) < MOTION_STEP_ANGLE) {
            spot_position.phi = goal_position.phi;
        } else {
            spot_position.set = true;
            spot_position.phi = MOTION_STEP_ALFA * goal_position.phi + (1.0 - MOTION_STEP_ALFA)  *spot_position.phi;
        }

        if (abs(goal_position.psi - spot_position.psi) < MOTION_STEP_ANGLE) {
            spot_position.psi = goal_position.psi;
        } else {
            spot_position.set = true;
            spot_position.psi = MOTION_STEP_ALFA * goal_position.psi + (1.0 - MOTION_STEP_ALFA)  * spot_position.psi;
        }

        if (abs(goal_position.xm - spot_position.xm) < MOTION_STEP_MOVEMENT) {
            spot_position.xm = goal_position.xm;
        } else {
            spot_position.set = true;
            spot_position.xm = MOTION_STEP_ALFA * goal_position.xm + (1.0 - MOTION_STEP_ALFA)  * spot_position.xm;
        }

        if (abs(goal_position.ym - spot_position.ym) < MOTION_STEP_MOVEMENT) {
            spot_position.ym = goal_position.ym;
        } else {
            spot_position.set = true;
            spot_position.ym = MOTION_STEP_ALFA * goal_position.ym + (1.0 - MOTION_STEP_ALFA)  * spot_position.ym;
        }

        if (abs(goal_position.zm - spot_position.zm) < MOTION_STEP_MOVEMENT) {
            spot_position.zm = goal_position.zm;
        } else {
            spot_position.set = true;
            spot_position.zm = MOTION_STEP_ALFA * goal_position.zm + (1.0 - MOTION_STEP_ALFA)  * spot_position.zm;
        }

        ESP_LOGI(tag, "CURRENT (%f,%f,%f - %f,%f,%f) %d", spot_position.omega, spot_position.phi, spot_position.psi, spot_position.xm, spot_position.ym, spot_position.zm, spot_position.set);
        
        esp_err_t ret = spot_IK(spot_position.omega*DEGREES2RAD, spot_position.phi*DEGREES2RAD, spot_position.psi*DEGREES2RAD, spot_position.xm, spot_position.ym, spot_position.zm, servo_angles_goal);
        ESP_LOGD(tag, "Valid IK %d", ret==ESP_OK);
        if (ret == ESP_OK) {
            print_int_matrix((int16_t*) servo_angles_goal, 4, 3, "servo_angles_goal", false);
            set_leg_servos();
        }

    } while (spot_position.set);


    set_new_orientation_act_value((int16_t) spot_position.omega, (int16_t) spot_position.phi, (int16_t) spot_position.psi, (int16_t) spot_position.xm, (int16_t) spot_position.ym, (int16_t) spot_position.zm);
}
#endif

void task_ik(void *ignore)
{
    ESP_LOGI(tag, "Executing on core %d", xPortGetCoreID());
    // esp_err_t ret;

    int i=0;
    for(i=0;i<12;i++)
        servo_conversion[i] = (servo_max[i] - servo_min[i]) / 180.0;

    reset_position();
    
#if 1
    while(1)
    {
        vTaskDelay(100 / portTICK_RATE_MS);
        if (goal_position.set) {
            goal_position.set = false;


            if(spot_state == SPOT_STATE_SLEEP)
                spot_sleep();
            else
                iterate_to_position();
            
            // esp_err_t ret = spot_IK(spot_position.omega*DEGREES2RAD, spot_position.phi*DEGREES2RAD, spot_position.psi*DEGREES2RAD, spot_position.xm, spot_position.ym, spot_position.zm, servo_angles_goal);
            // ESP_LOGD(tag, "Valid IK %d", ret==ESP_OK);
            // if (ret == ESP_OK) {
            //     spot_position = goal_position;
            //     print_int_matrix((int16_t*) servo_angles_goal, 4, 3, "servo_angles_goal", false);
            //     set_leg_servos();
            
            //     set_new_orientation_act_value((int16_t) spot_position.omega, (int16_t) spot_position.phi, (int16_t) spot_position.psi, (int16_t) spot_position.xm, (int16_t) spot_position.ym, (int16_t) spot_position.zm);
            // }

            set_leg_servos();
            set_new_orientation_act_value((int16_t) spot_position.omega, (int16_t) spot_position.phi, (int16_t) spot_position.psi, (int16_t) spot_position.xm, (int16_t) spot_position.ym, (int16_t) spot_position.zm);
        }


    }
#endif

    vTaskDelete(NULL);
}

void app_main()
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    init_pca9685();
    init_servos();

    ESP_LOGD(tag, "BLE");
    start_bluetooth();
    set_new_orientation_cb(&set_orientation_cb);

    // ESP_LOGI(tag, "Start Example.");

    // float A[3][3] = {{0, 1, 0} , {0, 1, 2}};
    // float B[4][2] = {{0, 0}, {1, 1}, {0, 1}};
    // float C[3][2];
    // dspm_mult_f33_ae32((float*) A, (float*) B,  (float*) C, 2, 2, 2);

    // ESP_LOGD(tag, "%.3f %.2f", C[0][0], C[0][1]);
    // ESP_LOGD(tag, "%.3f %.2f", C[1][0], C[1][1]);



    xTaskCreate(task_ik, "task_ik", 1025 * 2, (void* ) 0, 10, NULL);
}
