#ifndef ESP_PLATFORM
#include "std_msgs/Float32.h"
#include "std_msgs/Bool.h"
#include "std_msgs/String.h"
#include "std_msgs/Float32MultiArray.h"
#include "geometry_msgs/Vector3.h"
#else // ESP_PLATFORM
#include <memory>
#endif // ESP_PLATFORM

#include "spot_micro_motion_cmd.h"
#include "spot_micro_kinematics/spot_micro_kinematics.h"
#ifndef ESP_PLATFORM
#include "i2cpwm_board/Servo.h"
#include "i2cpwm_board/ServoArray.h"
#include "i2cpwm_board/ServoConfig.h"
#include "i2cpwm_board/ServosConfig.h"
#endif // ESP_PLATFORM
#include "spot_micro_idle.h"

#include <esp_log.h>
static char tag[] = "IKTEST";


using namespace smk;

// Constructor
SpotMicroMotionCmd::SpotMicroMotionCmd()
{

  if (smnc_.debug_mode) {
    std::cout<<"from Constructor \n";
  }

  // Initialize Command 
  cmd_ = Command();

  // Initialize state to Idle state
  state_ = std::make_unique<SpotMicroIdleState>();

  // Read in config parameters into smnc_
  readInConfigParameters();


  // Initialize spot micro kinematics object of this class
  sm_ = smk::SpotMicroKinematics(0.0f, 0.0f, 0.0f, smnc_.smc);

  // Set an initial body height and stance cmd for idle mode
  body_state_cmd_.euler_angs = {.phi = 0.0f, .theta = 0.0f, .psi = 0.0f};
  body_state_cmd_.xyz_pos = {.x = 0.0f, .y = smnc_.lie_down_height, .z = 0.0f};
  body_state_cmd_.leg_feet_pos = getLieDownStance();

  // Set the spot micro kinematics object to this initial command
  sm_.setBodyState(body_state_cmd_);
#if 0   //FIXME:
  // Initialize servo array message with 12 servo objects
  for (int i = 1; i <= smnc_.num_servos; i++) {
    i2cpwm_board::Servo temp_servo;
    temp_servo.servo = i;
    temp_servo.value = 0;
    servo_array_.servos.push_back(temp_servo);
  }

  // Initialize servo array absolute message with 12 servo object with a value of
  // zero, just copy servo_array_msg since it's already correct 
  servo_array_absolute_.servos = servo_array_.servos;
#endif // 0
#ifndef ESP_PLATFORM
  // Initialize publishers and subscribers
  // stand cmd event subscriber 
  stand_sub_ = nh.subscribe("/stand_cmd", 1, &SpotMicroMotionCmd::standCommandCallback, this);
    
  // idle cmd event subscriber
  idle_sub_ = nh.subscribe("/idle_cmd", 1, &SpotMicroMotionCmd::idleCommandCallback, this);

  // walk cmd event subscriber
  walk_sub_ = nh.subscribe("/walk_cmd", 1, &SpotMicroMotionCmd::walkCommandCallback, this);
 
  // speed command subscriber
  speed_cmd_sub_ = nh.subscribe("/speed_cmd", 1, &SpotMicroMotionCmd::speedCommandCallback, this);  

  // body angle command subscriber
  body_angle_cmd_sub_ = nh.subscribe("/angle_cmd", 1, &SpotMicroMotionCmd::angleCommandCallback, this);  

  // servos_absolute publisher
  servos_absolute_pub_ = nh.advertise<i2cpwm_board::ServoArray>("servos_absolute", 1);

  // Servos proportional publisher
  servos_proportional_pub_ = nh.advertise<i2cpwm_board::ServoArray>("servos_proportional",1);  
  
  // Servos configuration publisher
  servos_config_client_ = nh.serviceClient<i2cpwm_board::ServosConfig>("config_servos");

  // Body state publisher for plotting
  body_state_pub_ = nh.advertise<std_msgs::Float32MultiArray>("body_state",1);

  // State string publisher for lcd monitor
  sm_state_pub_ = nh.advertise<std_msgs::String>("sm_state",1);

  // Speed command state publisher for lcd monitor
  sm_speed_cmd_pub_ = nh.advertise<geometry_msgs::Vector3>("sm_speed_cmd",1);

  // Angle command state publisher for lcd monitor
  sm_angle_cmd_pub_ = nh.advertise<geometry_msgs::Vector3>("sm_angle_cmd",1);

  // Initialize lcd monitor messages
  state_string_msg_.data = "Idle";

  speed_cmd_msg_.x = 0.0f;
  speed_cmd_msg_.y = 0.0f;
  speed_cmd_msg_.z = 0.0f;
  
  angle_cmd_msg_.x = 0.0f;
  angle_cmd_msg_.y = 0.0f;
  angle_cmd_msg_.z = 0.0f;
 

  // Only do if plot mode
  // Initialize body state message for plot debug only
  // Initialize 18 values to hold xyz positions of the four legs (12) + 
  // the body x,y,z positions (3), and the body angles (3) for a total of 18
  if (smnc_.plot_mode) {
    for (int i = 0; i < 18; i++) {
      body_state_msg_.data.push_back(0.0f); 
    }
  }
#else // ESP_PLATFORM
    //TODO: ESP32 initilize hooks 
#endif // ESP_PLATFORM
}

// Destructor method
SpotMicroMotionCmd::~SpotMicroMotionCmd() {

  if (smnc_.debug_mode) {
    std::cout<<"from Destructor \n";
  }
  // Free up the memory assigned from heap
}

void SpotMicroMotionCmd::readInConfigParameters() {
    //TODO: hard code the parameters for now
#if 0
  // Read in and save parameters 
  // Use private node handle for getting params so just the relative
  // parameter name can be used (as opposed to the global name, e.g.:
  // /spot_micro_motion_cmd/param1
  pnh_.getParam("hip_link_length", smnc_.smc.hip_link_length);
  pnh_.getParam("upper_leg_link_length", smnc_.smc.upper_leg_link_length);
  pnh_.getParam("lower_leg_link_length", smnc_.smc.lower_leg_link_length);
  pnh_.getParam("body_width", smnc_.smc.body_width);
  pnh_.getParam("body_length", smnc_.smc.body_length);
  pnh_.getParam("default_stand_height", smnc_.default_stand_height);
  pnh_.getParam("stand_front_x_offset", smnc_.stand_front_x_offset);
  pnh_.getParam("stand_back_x_offset", smnc_.stand_back_x_offset);
  pnh_.getParam("lie_down_height", smnc_.lie_down_height);
  pnh_.getParam("lie_down_foot_x_offset", smnc_.lie_down_feet_x_offset);
  pnh_.getParam("num_servos", smnc_.num_servos);
  pnh_.getParam("servo_max_angle_deg", smnc_.servo_max_angle_deg);
  pnh_.getParam("transit_tau", smnc_.transit_tau);
  pnh_.getParam("transit_rl", smnc_.transit_rl);
  pnh_.getParam("transit_angle_rl", smnc_.transit_angle_rl);
  pnh_.getParam("dt", smnc_.dt);
  pnh_.getParam("debug_mode", smnc_.debug_mode);
  pnh_.getParam("plot_mode", smnc_.plot_mode);
  pnh_.getParam("max_fwd_velocity", smnc_.max_fwd_velocity);
  pnh_.getParam("max_side_velocity", smnc_.max_side_velocity);
  pnh_.getParam("max_yaw_rate", smnc_.max_yaw_rate);
  pnh_.getParam("z_clearance", smnc_.z_clearance);
  pnh_.getParam("alpha", smnc_.alpha);
  pnh_.getParam("beta", smnc_.beta);
  pnh_.getParam("num_phases", smnc_.num_phases);
  pnh_.getParam("rb_contact_phases", smnc_.rb_contact_phases);
  pnh_.getParam("rf_contact_phases", smnc_.rf_contact_phases);
  pnh_.getParam("lf_contact_phases", smnc_.lf_contact_phases);
  pnh_.getParam("lb_contact_phases", smnc_.lb_contact_phases);
  pnh_.getParam("overlap_time", smnc_.overlap_time);
  pnh_.getParam("swing_time", smnc_.swing_time);
  pnh_.getParam("foot_height_time_constant", smnc_.foot_height_time_constant);
  pnh_.getParam("body_shift_phases", smnc_.body_shift_phases);
  pnh_.getParam("fwd_body_balance_shift", smnc_.fwd_body_balance_shift);
  pnh_.getParam("back_body_balance_shift", smnc_.back_body_balance_shift);
  pnh_.getParam("side_body_balance_shift", smnc_.side_body_balance_shift);
  

  // Derived parameters
  // Round result of division of floats
  smnc_.overlap_ticks = round(smnc_.overlap_time / smnc_.dt);
  smnc_.swing_ticks = round(smnc_.swing_time / smnc_.dt);
  smnc_.stance_ticks = 7 * smnc_.swing_ticks;
  smnc_.overlap_ticks = round(smnc_.overlap_time / smnc_.dt);
  smnc_.phase_ticks = std::vector<int>
      {smnc_.swing_ticks, smnc_.swing_ticks, smnc_.swing_ticks, smnc_.swing_ticks,
       smnc_.swing_ticks, smnc_.swing_ticks, smnc_.swing_ticks, smnc_.swing_ticks};
  smnc_.phase_length = smnc_.num_phases * smnc_.swing_ticks;

  // Temporary map for populating map in smnc_
  std::map<std::string, float> temp_map;
  
  // Iterate over servo names, as defined in the map servo_cmds_rad, to populate
  // the servo config map in smnc_
  for(std::map<std::string, float>::iterator 
      iter = servo_cmds_rad_.begin();
      iter != servo_cmds_rad_.end();
      ++iter) {

    std::string servo_name = iter->first; // Get key, string of the servo name
    
    pnh_.getParam(servo_name, temp_map); // Read the parameter. Parameter name must match servo name
    smnc_.servo_config[servo_name] = temp_map; // Assing in servo config to map in the struct
  }
#endif //0
}

bool SpotMicroMotionCmd::publishServoConfiguration() {  
#ifndef ESP_PLATFORM
  // Create a temporary servo config
  i2cpwm_board::ServoConfig temp_servo_config;
  i2cpwm_board::ServosConfig temp_servo_config_array;

  // Loop through servo configuration dictionary in smnc_, append servo to
  for (std::map<std::string, std::map<std::string, float>>::iterator
       iter = smnc_.servo_config.begin();
       iter != smnc_.servo_config.end();
       ++iter) {

    std::map<std::string, float> servo_config_params = iter->second;
    temp_servo_config.center = servo_config_params["center"];
    temp_servo_config.range = servo_config_params["range"];
    temp_servo_config.servo = servo_config_params["num"];
    temp_servo_config.direction = servo_config_params["direction"];

    // Append to temp_servo_config_array
    temp_servo_config_array.request.servos.push_back(temp_servo_config);
  }

  // call the client service, return true if succesfull, false if not
  if (!servos_config_client_.call(temp_servo_config_array)) {
    if (!smnc_.debug_mode) {
      // Only error out if not in debug mode
      ROS_ERROR("Failed to call service servo_config");
      return false;
    }
  }
#else // ESP_PLATFORM
#endif // ESP_PLATFORM
  return true;
}

SpotMicroNodeConfig SpotMicroMotionCmd::getNodeConfig() {
  return smnc_;
}

LegsFootPos SpotMicroMotionCmd::getNeutralStance() {
  float len = smnc_.smc.body_length; // body length
  float width = smnc_.smc.body_width; // body width
  float l1 = smnc_.smc.hip_link_length; // liength of the hip link
  float f_offset = smnc_.stand_front_x_offset; // x offset for front feet in neutral stance
  float b_offset = smnc_.stand_back_x_offset; // y offset for back feet in neutral stance

  LegsFootPos neutral_stance;
  neutral_stance.right_back  = {.x = -len/2 + b_offset, .y = 0.0f, .z =  width/2 + l1};
  neutral_stance.right_front = {.x =  len/2 + f_offset, .y = 0.0f, .z =  width/2 + l1};
  neutral_stance.left_front  = {.x =  len/2 + f_offset, .y = 0.0f, .z = -width/2 - l1};
  neutral_stance.left_back   = {.x = -len/2 + b_offset, .y = 0.0f, .z = -width/2 - l1};

  return neutral_stance;
}

LegsFootPos SpotMicroMotionCmd::getLieDownStance() {
  float len = smnc_.smc.body_length; // body length
  float width = smnc_.smc.body_width; // body width
  float l1 = smnc_.smc.hip_link_length; // length of the hip link
  float x_off = smnc_.lie_down_feet_x_offset;

  LegsFootPos lie_down_stance;
  lie_down_stance.right_back  = {.x = -len/2 + x_off, .y = 0.0f, .z =  width/2 + l1};
  lie_down_stance.right_front = {.x =  len/2 + x_off, .y = 0.0f, .z =  width/2 + l1};
  lie_down_stance.left_front  = {.x =  len/2 + x_off, .y = 0.0f, .z = -width/2 - l1};
  lie_down_stance.left_back   = {.x = -len/2 + x_off, .y = 0.0f, .z = -width/2 - l1};

  return lie_down_stance;
}

void SpotMicroMotionCmd::publishZeroServoAbsoluteCommand() {
#ifndef ESP_PLATFORM
  // Publish the servo absolute message
  servos_absolute_pub_.publish(servo_array_absolute_);
#else // ESP_PLATFORM
#endif // ESP_PLATFORM
}

void SpotMicroMotionCmd::handleInputCommands() {
  // Delegate input handling to state
  state_->handleInputCommands(sm_.getBodyState(), smnc_, cmd_, this, &body_state_cmd_);
}


void SpotMicroMotionCmd::changeState(std::unique_ptr<SpotMicroState> sms) {
  // Change the active state
  state_ = std::move(sms);

  // Call init method of new state
  state_->init(sm_.getBodyState(), smnc_, cmd_, this);

  // Reset all command values
  cmd_.resetAllCommands();
}

void SpotMicroMotionCmd::setServoCommandMessageData() {

  // Set the state of the spot micro kinematics object by setting the foot
  // positions, body position, and body orientation. Retrieve joint angles and
  // set the servo cmd message data
  sm_.setBodyState(body_state_cmd_);
  LegsJointAngles joint_angs = sm_.getLegsJointAngles();

  // Set the angles for the servo command message
  servo_cmds_rad_["RF_1"] = joint_angs.right_front.ang1;
  servo_cmds_rad_["RF_2"] = joint_angs.right_front.ang2;
  servo_cmds_rad_["RF_3"] = joint_angs.right_front.ang3;
 
  servo_cmds_rad_["RB_1"] = joint_angs.right_back.ang1;
  servo_cmds_rad_["RB_2"] = joint_angs.right_back.ang2;
  servo_cmds_rad_["RB_3"] = joint_angs.right_back.ang3;
 
  servo_cmds_rad_["LF_1"] = joint_angs.left_front.ang1;
  servo_cmds_rad_["LF_2"] = joint_angs.left_front.ang2;
  servo_cmds_rad_["LF_3"] = joint_angs.left_front.ang3;
 
  servo_cmds_rad_["LB_1"] = joint_angs.left_back.ang1;
  servo_cmds_rad_["LB_2"] = joint_angs.left_back.ang2;
  servo_cmds_rad_["LB_3"] = joint_angs.left_back.ang3;
}

void SpotMicroMotionCmd::publishServoProportionalCommand() {
  for (std::map<std::string, std::map<std::string, float>>::iterator
       iter = smnc_.servo_config.begin();
       iter != smnc_.servo_config.end();
       ++iter) {
 
    std::string servo_name = iter->first;
    std::map<std::string, float> servo_config_params = iter->second;
    
//TODO:int servo_num = servo_config_params["num"];
    float cmd_ang_rad = servo_cmds_rad_[servo_name];
    float center_ang_rad = servo_config_params["center_angle_deg"]*M_PI/180.0f;
    float servo_proportional_cmd = (cmd_ang_rad - center_ang_rad) /
                                   (smnc_.servo_max_angle_deg*M_PI/180.0f);
 
    if (servo_proportional_cmd > 1.0f) {
      servo_proportional_cmd = 1.0f;
      ESP_LOGW(tag, "Proportional Command above +1.0 was computed, clipped to 1.0");
      ESP_LOGW(tag, "Joint %s, Angle: %1.2f", servo_name.c_str(), cmd_ang_rad*180.0/M_PI);
 
    } else if (servo_proportional_cmd < -1.0f) {
      servo_proportional_cmd = -1.0f;
      ESP_LOGW(tag, "Proportional Command below -1.0 was computed, clipped to -1.0");
      ESP_LOGW(tag, "Joint %s, Angle: %1.2f", servo_name.c_str(), cmd_ang_rad*180.0/M_PI);
    }
#ifndef ESP_PLATFORM
    servo_array_.servos[servo_num-1].servo = servo_num;
    servo_array_.servos[servo_num-1].value = servo_proportional_cmd; 
    // Publish message
    servos_proportional_pub_.publish(servo_array_);
#else // ESP_PLATFORM
    //TODO:
#endif // ESP_PLATFORM
 }
}