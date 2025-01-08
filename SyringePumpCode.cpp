#include <AccelStepper.h>


// Pin definitions
const int dirPin = 2;
const int stepPin = 3;
const int startPauseButtonPin = 4;
const int clockwiseButtonPin = 5;
const int counterClockwiseButtonPin = 6;
const int limitSwitchPin = 7;
const int potentiometerPin = A0;
const int redLEDPin = 8;
const int greenLEDPin = 9;
const int blueLEDPin = 10;


// Global variables
AccelStepper stepper(AccelStepper::DRIVER, stepPin, dirPin);
bool isRunning = false; // State of the syringe pump
int lastPotValue = -1; // Previous potentiometer value
float flowRate = 1.0;   // Default flow rate in ml/min
//float syringeDiameter = 14.5; // Default syringe diameter in mm for 10mL syringe
float altSyringeDiameter = 19.1; // Alternate syringe diameter in mm for 20mL syringe
float maxSpeed = 1000;  // Max speed for the stepper motor (steps/second)
int stepsPerRevolution = 200; // Steps per revolution of the stepper motor
float leadScrewPitch = 2.0; // Lead screw pitch in mm/revolution

float syringeDiameter = altSyringeDiameter;

// Microstepping configuration
const int microsteppingFactor = 16; // 1/16 microstepping enabled
const float syringeAreaFactor = 3.14159 / 4.0; // Pi/4
const float stepsPerMM = (stepsPerRevolution * microsteppingFactor) / leadScrewPitch;


// Precomputed variables for motor speed
float motorSpeed = 0; // Motor speed in steps/sec


void setup() {
 Serial.begin(9600);


 // Configure pins
 pinMode(startPauseButtonPin, INPUT_PULLUP);
 pinMode(clockwiseButtonPin, INPUT_PULLUP);
 pinMode(counterClockwiseButtonPin, INPUT_PULLUP);
 pinMode(limitSwitchPin, INPUT_PULLUP);
 pinMode(redLEDPin, OUTPUT);
 pinMode(greenLEDPin, OUTPUT);
 pinMode(blueLEDPin, OUTPUT);

 lastPotValue = analogRead(potentiometerPin);

 // Initialize stepper motor
 stepper.setMaxSpeed(maxSpeed);


 // Update LEDs to initial state
 updateLEDs();
}


void loop() {
 // Check if the flow rate has changed
 checkAndUpdateFlowRate();


 // Handle start/pause button press
 handleStartPause();


 // Stop the motor if the limit switch is triggered
 if (digitalRead(limitSwitchPin) == LOW) {
   isRunning = false;
   updateLEDs();
 }


 // Handle manual movement buttons
 if (digitalRead(clockwiseButtonPin) == LOW) {
   manualMove(true); // Move clockwise
 } else if (digitalRead(counterClockwiseButtonPin) == LOW) {
   manualMove(false); // Move counterclockwise
 }


 // Run the motor if the pump is running
 if (isRunning) {
   stepper.setSpeed(motorSpeed); // Use precomputed motor speed
   stepper.runSpeed();
 }
}


// Check if the potentiometer value has changed and update the flow rate
void checkAndUpdateFlowRate() {
 int currentPotValue = analogRead(potentiometerPin); // Read potentiometer value

 if (abs(currentPotValue-lastPotValue) >= 20) { // If value has changed
   lastPotValue = currentPotValue;


   // Map potentiometer value to flow rate (0.1 to 5.0 ml/min)
   flowRate = map(currentPotValue, 0, 1023, 1, 51) / 10.0; // Scale to 0.1 to 5.0 ml/min


   // Update motor speed based on the new flow rate
   motorSpeed = calculateSpeedFromFlowRate(flowRate, syringeDiameter);


   // Output flow rate to serial monitor
   Serial.print("Flow Rate: ");
   Serial.print(flowRate);
   Serial.println(" ml/min");
 }else{
   motorSpeed = motorSpeed;
 }
}


// Calculate the motor speed from flow rate and syringe diameter
float calculateSpeedFromFlowRate(float flowRate, float syringeDiameter) {
 float syringeArea = syringeAreaFactor * syringeDiameter * syringeDiameter; // Area in mm^2
 float flowRatePerSecond = flowRate / 60.0; // Flow rate in ml/s
 float plungerSpeed = (flowRatePerSecond * 1000.0) / syringeArea; // Plunger speed in mm/s
 float motorSpeed = plungerSpeed * stepsPerMM; // Motor speed in steps/s
 return constrain(motorSpeed, 0, maxSpeed); // Ensure it doesn't exceed maxSpeed
}


// Handle start/pause button to toggle running state
void handleStartPause() {
 static bool lastState = false; // Keeps track of the last running state
 // Read the button state directly
 bool buttonState = digitalRead(startPauseButtonPin);
 if (buttonState == LOW) { // Button is pressed (LOW)
   if (!lastState) { // Only toggle if the state hasn't been toggled yet
     isRunning = !isRunning; // Toggle running state
     updateLEDs();
     lastState = true; // Mark the state as toggled
   }
 } else { // Button is released (HIGH)
   lastState = false; // Reset the toggle state
 }
}


// Manually move the motor
void manualMove(bool clockwise) {
 stepper.setSpeed(clockwise ? maxSpeed : -maxSpeed);
 stepper.runSpeed();
}


// Update the LED colors based on the current state
void updateLEDs() {
 if (!isRunning) {
   // Paused or stopped
   digitalWrite(redLEDPin, 255);
   digitalWrite(greenLEDPin, 255);
   digitalWrite(blueLEDPin, 0); // Yellow LED when paused
 } else {
   // Running
   digitalWrite(redLEDPin, 0);
   digitalWrite(greenLEDPin, 255);
   digitalWrite(blueLEDPin, 0); // Green LED when running
 }
 if (digitalRead(limitSwitchPin) == LOW) {
   // Out of liquid or limit reached
   digitalWrite(redLEDPin, 255);
   digitalWrite(greenLEDPin, 0);
   digitalWrite(blueLEDPin, 0); // Red LED when out of liquid
 }
}
