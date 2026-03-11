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
#define switchPin 6
#define giveWirePin 7
#define pickupWirePin 8
#define motorInterfaceType 1

// Physical constants
const float diameter = 0.17;       // drum diameter in meters
const float pulsesPerRevolution = 1200.0; // encoder configuration (600 lines * 2)
const float metersPerPulse = (PI * diameter) / pulsesPerRevolution; 

// Variables
volatile long encoderPulses = 0;
volatile int lastStateA; 
float current_L = 0.0; // current length in meters
float ref_L = 0.0;     // target length received from ROS
bool enable = false;   // safety enable
bool one_way = false;  // release-only mode
bool switch_state = false; // switch state

// Objects
AccelStepper stepper = AccelStepper(motorInterfaceType, stepPin, dirPin);
ros::NodeHandle nh;

// ROS messages
std_msgs::Float32 length_msg;      // To publish current length
std_msgs::Bool length_reached_msg; // To notify target reached
std_msgs::Bool switch_msg;         // To publish switch state

// Publishers
ros::Publisher pub_length("tie_controller/length_status", &length_msg);
ros::Publisher pub_reached("tie_controller/length_reached", &length_reached_msg);
ros::Publisher pub_switch("tie_controller/switch_state", &switch_msg);

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
  if (fabs(ref_L - newTarget) > 0.02) {
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
  pinMode(switchPin, INPUT_PULLUP);
  pinMode(giveWirePin, INPUT_PULLUP);
  pinMode(pickupWirePin, INPUT_PULLUP);
  // Stepper pins (4 and 5) are configured by AccelStepper
  
  // Encoder interrupt
  lastStateA = digitalRead(encoderPinA);
  attachInterrupt(digitalPinToInterrupt(encoderPinA), readEncoder, CHANGE);

  // Stepper motor configuration
  stepper.setMinPulseWidth(20);
  stepper.setMaxSpeed(400);      // Speed
  stepper.setAcceleration(400);   // Acceleration

  // ROS init
  nh.initNode();
  nh.advertise(pub_length);
  nh.advertise(pub_reached);
  nh.advertise(pub_switch);
  nh.subscribe(sub_len);
  nh.subscribe(sub_enable);
  nh.subscribe(sub_reset);
  nh.subscribe(sub_oneway);

  // Read initial switch state (message logged in loop after ROS connects)
  switch_state = (digitalRead(switchPin) == LOW);
  enable = switch_state;
}

void loop() {
  // --- Log initial switch state once ROS is connected ---
  static bool startupLogged = false;
  if (!startupLogged && nh.connected()) {
    startupLogged = true;
    if (enable) {
      nh.loginfo("Startup: Motor ENABLED (switch ON)");
    } else {
      nh.loginfo("Startup: Motor DISABLED (switch OFF)");
    }
    switch_msg.data = enable;
    pub_switch.publish(&switch_msg);
  }

  // --- Switch reading (on/off toggle switch) ---
  // INPUT_PULLUP: switch closed (ON) = LOW, switch open (OFF) = HIGH
  bool switchOn = (digitalRead(switchPin) == LOW);

  if (switchOn != switch_state) {
    switch_state = switchOn;
    enable = switchOn;

    if (enable) {
      nh.loginfo("Switch: Motor ENABLED");
    } else {
      stepper.stop();
      nh.loginfo("Switch: Motor DISABLED");
    }

    switch_msg.data = enable;
    pub_switch.publish(&switch_msg);
  }

  // Calculate actual length based on Encoder
  current_L = (float)encoderPulses * metersPerPulse;

  // Movement logic with anti-oscillation
  static bool atTarget = true;           // Motor state
  static bool wasManualOverride = false;  // Track manual button state
  
  // --- Manual push-button override ---
  // INPUT_PULLUP: button pressed = LOW, released = HIGH
  bool giveWirePressed = (digitalRead(giveWirePin) == LOW);
  bool pickupWirePressed = (digitalRead(pickupWirePin) == LOW);
  
  if (enable) {
    // Only one button at a time — if both pressed, ignore
    bool manualActive = (giveWirePressed || pickupWirePressed) && !(giveWirePressed && pickupWirePressed);
    if (manualActive) {
      // Manual override: spin motor while button is held
      if (giveWirePressed) {
        stepper.setSpeed(stepper.maxSpeed());   // Give cable direction
      } else {
        stepper.setSpeed(-stepper.maxSpeed());  // Pickup cable direction
      }
      stepper.runSpeed();
      wasManualOverride = true;
      atTarget = true;
    } else {
      // If we just released a button, update ref_L to current position
      // so the motor doesn't jump back to the old target
      if (wasManualOverride) {
        ref_L = current_L;
        wasManualOverride = false;
        length_reached_msg.data = true;
        pub_reached.publish(&length_reached_msg);
      }
      
      // Normal target-following logic
      // Calculate target in pulses
      long targetPulses = (long)(ref_L / metersPerPulse);
      long error = targetPulses - encoderPulses;
      
      // Hysteresis zones
      const int DEAD_ZONE = 50;     // To stop (~2cm tolerance)
      const int START_ZONE = 100;    // To start moving again (~4cm)
      
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
          // Keep moving at configured speed
          if (error > 0) {
            stepper.setSpeed(stepper.maxSpeed());
          } else {
            stepper.setSpeed(-stepper.maxSpeed());
          }
          stepper.runSpeed();
        }
      }
    }
  } else {
    stepper.stop();
    atTarget = true;
    wasManualOverride = false;
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