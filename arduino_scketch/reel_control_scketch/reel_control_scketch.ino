// Define Libraries
#include <AccelStepper.h>
#define nullptr NULL
#include <ros.h>
#include <std_msgs/Bool.h>
#include <std_msgs/UInt16.h>
#include <std_msgs/Float32.h>

// Pin configuration
#define encoderPinA 2  // green wire
#define encoderPinB 3  // white wire
#define stepPin 4
#define dirPin 5
#define motorInterfaceType 1

// Physical constants
const float diameter = 0.15;       // drum diameter in meters
const float pulsesPerRevolution = 1200.0; // encoder configuration (600 lines * 2)
const float metersPerPulse = (PI * diameter) / pulsesPerRevolution; 

// Variables
volatile long encoderPulses = 0;
volatile int lastStateA; 
float current_L = 0.0; // current length in meters
float ref_L = 0.0;     // target length received from ROS
bool enable = false;   // safety enable
bool one_way = false;  // release-only mode

// Objects
AccelStepper stepper = AccelStepper(motorInterfaceType, stepPin, dirPin);
ros::NodeHandle nh;

// ROS messages
std_msgs::Float32 length_msg;      // To publish current length
std_msgs::Bool length_reached_msg; // To notify target reached

// Publishers
ros::Publisher pub_length("tie_controller/length_status", &length_msg);
ros::Publisher pub_reached("tie_controller/length_reached", &length_reached_msg);

// Interrupt function (Encoder)
void readEncoder() {
  int currentA = digitalRead(encoderPinA);
  if (currentA != lastStateA) {
    if (digitalRead(encoderPinB) != currentA) {
      encoderPulses++;
    } else {
      encoderPulses--;
    }
  }
  lastStateA = currentA;
}

// Callbacks

// Receive new target length
void lengthSubCallback(const std_msgs::Float32& length_ref_msg) {
  float newTarget = length_ref_msg.data;

  // Safety filters
  if (newTarget < 0 || newTarget > 25) {
     nh.loginfo("Error: Length out of range (0-25m)");
     return;
  }
  
  if (one_way && newTarget < current_L) {
     nh.loginfo("One-Way mode active: Cannot retract cable.");
     return;
  }

  // If there is a significant change in the command
  if (abs(ref_L - newTarget) > 0.02) {
    ref_L = newTarget;
    
    // Conversion: Meters -> Encoder pulses
    // Calculate which pulse count corresponds to that distance
    long targetPulses = (long)(ref_L / metersPerPulse);
  
    // Simplification for AccelStepper:
    // If encoder controls, move motor towards target
    length_reached_msg.data = false;
    pub_reached.publish(&length_reached_msg);
    nh.loginfo("New target received.");
  }
}

// Enable/disable motor
void enableCallback(const std_msgs::Bool& bool_msg) {
  enable = bool_msg.data;
  if (!enable) {
    stepper.stop(); // Soft stop
    nh.loginfo("Motor DISABLED");
  } else {
    nh.loginfo("Motor ENABLED");
  }
}

// Reset length estimation 
void resetLengthCallback(const std_msgs::Float32& length_reset) {
  // Adjust encoder pulses to match the reset length
  float meters = length_reset.data;
  encoderPulses = (long)(meters / metersPerPulse);
  current_L = meters;
  ref_L = meters;
  stepper.setCurrentPosition(stepper.currentPosition()); // Reset motor too
  nh.loginfo("Length manually reset.");
}

// One way mode
void one_wayCallback(const std_msgs::Bool& bool_msg) {
  one_way = bool_msg.data;
}

// Subscribers
ros::Subscriber<std_msgs::Float32> sub_len("tie_controller/set_length", &lengthSubCallback);
ros::Subscriber<std_msgs::Bool> sub_enable("tie_controller/enable", &enableCallback);
ros::Subscriber<std_msgs::Float32> sub_reset("tie_controller/reset_length_estimation", &resetLengthCallback);
ros::Subscriber<std_msgs::Bool> sub_oneway("tie_controller/one_way", &one_wayCallback);

void setup() {
  // Pin configuration
  pinMode(encoderPinA, INPUT_PULLUP);
  pinMode(encoderPinB, INPUT_PULLUP);
  // Stepper pins (4 and 5) are configured by AccelStepper
  
  // Encoder interrupt
  lastStateA = digitalRead(encoderPinA);
  attachInterrupt(digitalPinToInterrupt(encoderPinA), readEncoder, CHANGE);

  // Stepper motor configuration
  stepper.setMaxSpeed(500);      // Speed
  stepper.setAcceleration(800);  // Acceleration

  // ROS init
  nh.initNode();
  nh.advertise(pub_length);
  nh.advertise(pub_reached);
  nh.subscribe(sub_len);
  nh.subscribe(sub_enable);
  nh.subscribe(sub_reset);
  nh.subscribe(sub_oneway);
}

void loop() {
  // Calculate actual length based on Encoder
  current_L = (float)encoderPulses * metersPerPulse;

  // Movement logic with anti-oscillation
  static bool atTarget = true;  // Motor state
  
  if (enable) {
    // Calculate target in pulses
    long targetPulses = (long)(ref_L / metersPerPulse);
    long error = targetPulses - encoderPulses;
    
    // Hysteresis zones
    const int DEAD_ZONE = 25;     // To stop (~1cm tolerance)
    const int START_ZONE = 50;    // To start moving again
    
    if (atTarget) {
      // Already at target - only move if error is large
      if (abs(error) > START_ZONE) {
        atTarget = false;
        length_reached_msg.data = false;
        pub_reached.publish(&length_reached_msg);
      }
    } else {
      // Moving towards target
      if (abs(error) <= DEAD_ZONE) {
        stepper.stop();
        stepper.setCurrentPosition(stepper.currentPosition());
        atTarget = true;
        
        // Notify ROS
        length_reached_msg.data = true;
        pub_reached.publish(&length_reached_msg);
      } else {
        // Keep moving
        if (error > 0) {
          stepper.moveTo(stepper.currentPosition() + 500);
        } else {
          stepper.moveTo(stepper.currentPosition() - 500);
        }
        stepper.run();
      }
    }
  } else {
    stepper.stop();
    atTarget = true;
  }

  // ROS communication
  static unsigned long lastPub = 0;
  if (millis() - lastPub > 100) { // 10 Hz
    length_msg.data = current_L;
    pub_length.publish(&length_msg);
    nh.spinOnce();
    lastPub = millis();
  }
}