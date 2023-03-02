/*
 * rosserial Controlling DC motor for REEL System
 * "control length cable"
 */
#define nullptr NULL
#define ENCODER_A       2  // Identify with cable color

#include <ros.h>
#include <std_msgs/String.h>
#include <std_msgs/Bool.h>
#include <std_msgs/UInt16.h>
#include <std_msgs/Float32.h>

// ROS stuff
ros::NodeHandle  nh;
std_msgs::Bool length_reached_msg;
std_msgs::Float32 length_msg;
std_msgs::Float32 error_msg;
ros::Publisher pub_length_reached("tie_controller/length_reached", &length_reached_msg); // Let the mission controller know that the cable as reached the desired length
ros::Publisher pub_error("tie_controller/error_status", &error_msg); // Para depurar el funcionamiento del cacharro
ros::Publisher pub_length("tie_controller/length_status", &length_msg);   // Emit the current estimated longitude of the tie


// Arduino stuff
const int pinIN1 = 7; // pin for spin direction
const int pinIN2 = 8; // pin for spin direction
const int pinENA = 10; // PWM to control velocity 

const int pinMotor[3] = { pinENA, pinIN1, pinIN2 };


// Control stuff
const float diameter = 0.1; //original 0.15
const int pulse_per_loop = 460; // number of encoder pulse in one loop 
const float meter_per_loop = 0.45; //Relation between tether length(mts) per reel loop
const float tolerance_error = 0.02; //min error to control Reel
bool controlling = false;
float initial_L = 5.0; //initial length 
float current_L = 5.0; //initial length 
float ref_L = initial_L; // reference length to control Must be initialize as initial_L value to not move from the begining
int sign = 1;

float error_init = 0.0;
int PWM = 0;
int PWM_max = 105;
int PWM_min = 80;
float error= 0.0;
volatile long int encoder_pos =  (int)(((float)pulse_per_loop) * initial_L/meter_per_loop); //create variable and give initial value

// *** Here all the Functions ***
void moveForward(const int pinMotor[3], int speed)
{
   digitalWrite(pinMotor[1], LOW);
   digitalWrite(pinMotor[2], HIGH);
   analogWrite(pinMotor[0], speed);
}

void moveBackward(const int pinMotor[3], int speed)
{
   digitalWrite(pinMotor[1], HIGH);
   digitalWrite(pinMotor[2], LOW);
   analogWrite(pinMotor[0], speed);
}

void fullStop(const int pinMotor[3])
{
   digitalWrite(pinMotor[1], LOW);
   digitalWrite(pinMotor[2], LOW);
   analogWrite(pinMotor[0], 0);
}

// *** Here all the Callbacks Declarations ***

void lengthSubCallback(const std_msgs::Float32& length_ref_msg) {
    if ( length_ref_msg.data <= 2.0 || length_ref_msg.data > 25) {
      fullStop(pinMotor);
      return;
    }

    ref_L = length_ref_msg.data;
    controlling  = true;
    if ( 0.0 < (ref_L - current_L))
      sign = 1;
    else
      sign = -1;
      length_reached_msg.data = false;
      pub_length_reached.publish(&length_reached_msg);
    
    error_init = ref_L - current_L;
}

void resetLengthSubCallback(const std_msgs::Float32& length_reset) {
     nh.loginfo("HOLA 11");
     current_L = initial_L = ref_L = length_reset.data;
     encoder_pos =  (int)(((float)pulse_per_loop) * length_reset.data/meter_per_loop);
     error_msg.data = 0.0;
     pub_error.publish(&error_msg);
     controlling = false;
     fullStop(pinMotor);
     nh.loginfo("HOLAAA 2");
}

ros::Subscriber<std_msgs::Float32> sub("tie_controller/set_length", &lengthSubCallback );   // Subscriber for the length command
ros::Subscriber<std_msgs::Float32> sub_reset("tie_controller/reset_length_estimation", &resetLengthSubCallback);         // Length reset topic just in case

void encoder(){
    if (sign == 1){
      encoder_pos++;
    }else{
      encoder_pos--;
    }
}

void controlReel(float ref_L_){
  // put your main code here, to run repeatedly:
  error =  ref_L_ - current_L;

  if (error > tolerance_error || error < -tolerance_error)
    PWM = (int)(((PWM_max- PWM_min)/(error_init-tolerance_error))*(error- tolerance_error)+ PWM_min);
  else
    PWM = 0;
  
  if ( 0.0 < error)
    sign = 1;
  else
    sign = -1;

  error_msg.data = error;
  pub_error.publish(&error_msg);

  if(current_L > 2.0 && current_L < 25.0){
    if (error < tolerance_error && error > -tolerance_error && controlling){
      length_reached_msg.data = true;
      controlling = false;
      pub_length_reached.publish(&length_reached_msg);
      fullStop(pinMotor);
    }
    else if (error > tolerance_error && controlling){ // increase length
      if (error > 1.0){
        moveForward(pinMotor, PWM);
      }
      else{
        moveForward(pinMotor, PWM);
      }
    }
    else if ( error < tolerance_error && controlling){ // reduce lentgh
      if (error < -1.0){
        moveBackward(pinMotor, PWM);
      }
      else{
        moveBackward(pinMotor, PWM);
      }
    }
  }
}

void setup()
{
   pinMode(pinIN1, OUTPUT);
   pinMode(pinIN2, OUTPUT);
   pinMode(pinENA, OUTPUT);
   pinMode(ENCODER_A, INPUT_PULLUP);

   attachInterrupt(digitalPinToInterrupt(ENCODER_A), encoder, RISING);
 
   // ROS Config
   nh.initNode();
   nh.advertise(pub_length);
   nh.advertise(pub_length_reached);
   nh.advertise(pub_error);
   nh.subscribe(sub);
   nh.subscribe(sub_reset);
}

void loop()
{
  current_L = (((float)(encoder_pos)* meter_per_loop))/(float)pulse_per_loop;
   
  controlReel(ref_L);     
  
  length_msg.data = current_L;
  pub_length.publish(&length_msg);
  nh.spinOnce();
}
