#include <Servo.h>
#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>
 
// Servo setup
Servo escServo;
Servo steeringServo;
Servo eyesServo;
 
// Pin assignments for ESC, steering, "eyes," DFPlayer, and sensors
const int escPin = 7;             // ESC on pin 7
const int steeringPin = 12;       // Steering servo on pin 12
const int eyesServoPin = 2;       // "Eyes" servo on pin 2
const int frontTrigPin = 8;       // Front sensor trig pin
const int frontEchoPin = 9;       // Front sensor echo pin
const int leftTrigPin = 3;
const int leftEchoPin = 4;
const int rightTrigPin = 5;
const int rightEchoPin = 6;
const int button1Pin = A0;        // Button 1 for controlling songs
const int button2Pin = A1;        // Button 2 for starting movement
 
// Distance thresholds for detection
const int frontThresholdDistance = 30;   // Front obstacle detection threshold
const int sideThresholdDistance = 15;    // Side distance for steering adjustment
const int maxStopCount = 3;              // Maximum consecutive stops before reversing
 
// DFPlayer setup
SoftwareSerial mySerial(10, 11);         // RX on pin 10, TX on pin 11
DFRobotDFPlayerMini myDFPlayer;
int songNumber = 1;                      // Start with song 1
const int totalSongs = 6;                // Set the total number of songs
 
// Control flags
bool startMoving = false;                // Flag to start the car movement after button press
int stopCount = 0;                       // Counter for consecutive stops
 
void setup() {
  Serial.begin(9600);
 
  // DFPlayer setup
  mySerial.begin(9600);
  pinMode(button1Pin, INPUT_PULLUP);     // Button 1 setup with internal pull-up resistor
  pinMode(button2Pin, INPUT_PULLUP);     // Button 2 setup with internal pull-up resistor
  delay(2000);                           // Small delay before initializing DFPlayer
 
  if (!myDFPlayer.begin(mySerial)) {
    Serial.println("DFPlayer Mini not detected.");
    while (true);
  }
 
  myDFPlayer.volume(25);                 // Set volume level
  Serial.println("DFPlayer Mini initialized.");
  myDFPlayer.play(songNumber);           // Start by playing the first song on power-up
  Serial.println("Playing t61first song...");
 
  // Attach ESC, steering, and eyes servos
  escServo.attach(escPin);
  steeringServo.attach(steeringPin);
  eyesServo.attach(eyesServoPin);
 
  escServo.write(90);                    // Neutral position initially (parked)
  steeringServo.write(90);               // Center steering
  eyesServo.write(90);                   // Center "eyes"
  delay(3000);                           // Allow ESC to initialize
 
  // Setup sensor pins
  pinMode(frontTrigPin, OUTPUT);
  pinMode(frontEchoPin, INPUT);
  pinMode(leftTrigPin, OUTPUT);
  pinMode(leftEchoPin, INPUT);
  pinMode(rightTrigPin, OUTPUT);
  pinMode(rightEchoPin, INPUT);
 
  Serial.println("Waiting for manual start...");
}
 
void loop() {
  static bool button1Pressed = false;
  static bool button2Pressed = false;
 
  // Check if button 1 has been pressed to change songs
  if (digitalRead(button1Pin) == LOW) { // Button 1 is pressed
    if (!button1Pressed) {              // Only process on the first press
      button1Pressed = true;            // Mark button as pressed
      delay(500);                       // Debounce delay
 
      // Change the song on button 1 press
      songNumber++;                     // Move to the next song
      if (songNumber > totalSongs) songNumber = 1; // Loop back to song 1 if we exceed totalSongs
      myDFPlayer.play(songNumber);      // Play the selected song
      Serial.print("Playing song: ");
      Serial.println(songNumber);
    }
  } else {
    button1Pressed = false;             // Reset when button is released
  }
 
  // Check if button 2 has been pressed to start movement
  if (digitalRead(button2Pin) == LOW && !startMoving) { // Button 2 is pressed to start movement
    if (!button2Pressed) {              // Only process on the first press
      button2Pressed = true;            // Mark button as pressed
      delay(500);                       // Debounce delay
      startMoving = true;               // Set the flag to start movement
      Serial.println("Starting movement...");
    }
  } else {
    button2Pressed = false;             // Reset when button is released
  }
 
  // If not started, keep the car in neutral and return
  if (!startMoving) {
    escServo.write(90);                 // Ensure ESC remains in neutral
    return;
  }
 
  // Continuous signal refresh for ESC for forward motion after starting
  escServo.write(100);                  // Forward motion
 
  // Measure distances
  int frontDistance = measureDistance(frontTrigPin, frontEchoPin);
  int leftDistance = measureDistance(leftTrigPin, leftEchoPin);
  int rightDistance = measureDistance(rightTrigPin, rightEchoPin);
 
  // Print distances for monitoring
  Serial.print("Front: ");
  Serial.print(frontDistance);
  Serial.print(" cm | Left: ");
  Serial.print(leftDistance);
  Serial.print(" cm | Right: ");
  Serial.println(rightDistance);
 
  // Stop and avoid obstacle if a front obstacle is detected
  if (frontDistance <= frontThresholdDistance) {
    escServo.write(90); // Stop
    Serial.println("Stopping due to front obstacle");
 
    // Increment stop counter
    stopCount++;
 
    // Check if the car should reverse after multiple stops
    if (stopCount >= maxStopCount) {
      reverseAndTurn(); // Reverse and turn around
      stopCount = 0;    // Reset stop counter after maneuver
      return;
    }
 
    // Scan with "eyes" to find the clearer path
    int leftScan = scanDirection(45);    // Look left
    int rightScan = scanDirection(135);  // Look right
 
    // Decide direction based on scan results
    if (leftScan > rightScan) {
      Serial.println("Turning left to avoid obstacle.");
      gradualTurn(120);  // Turn wheels left
    } else {
      Serial.println("Turning right to avoid obstacle.");
      gradualTurn(60);   // Turn wheels right
    }
 
    // Move forward briefly to clear the obstacle
    escServo.write(100);           // Move forward
    delay(1000);                   // Move forward for a moment
 
    // Center the wheels, reset to neutral, and resume forward motion
    gradualTurn(90);               // Center steering
    escServo.write(90);            // Neutral stop for clear reset
    delay(500);                    // Pause briefly
    escServo.write(100);           // Resume forward motion
  } else {
    Serial.println("Moving Forward");
 
    // Reset stop counter if moving forward without obstacles
    stopCount = 0;
 
    // Adjust steering based on side sensors to keep the car aligned
    if (leftDistance <= sideThresholdDistance) {
      Serial.println("Adjusting Steering: Turning Right");
      gradualTurn(120);  // Turn slightly right
    } else if (rightDistance <= sideThresholdDistance) {
      Serial.println("Adjusting Steering: Turning Left");
      gradualTurn(60);   // Turn slightly left
    } else {
      gradualTurn(90);   // Center the steering
      Serial.println("Steering Centered");
    }
  }
 
  delay(500); // Delay before the next reading
}
 
// Function to reverse and perform a three-point turn if too many consecutive stops
void reverseAndTurn() {
  Serial.println("Attempting a three-point turn due to repeated obstacles");
 
  // Step 1: Reverse with wheels turned left
  escServo.write(90);               // Stop the car
  steeringServo.write(60);          // Turn wheels left
  delay(1000);                      // Short pause
  escServo.write(85);               // Slightly below neutral to reset ESC
  delay(500);
  escServo.write(90);               // Neutral again
  delay(3000);
 
  escServo.write(80);               // Reverse motion
  delay(2000);                      // Move backward for a short distance
 
  // Step 2: Stop, turn wheels right, and move forward
  escServo.write(90);               // Stop again
  delay(1000);
  steeringServo.write(120);         // Turn wheels right
  delay(500);
  escServo.write(100);              // Move forward
  delay(2000);                      // Move forward for a short distance
 
  // Step 3: Center wheels and resume forward motion
  escServo.write(90);               // Stop to reset
  delay(1000);
  steeringServo.write(90);          // Center the wheels
  delay(500);
  escServo.write(100);              // Resume forward motion
 
  Serial.println("Completed three-point turn, resuming forward motion");
}
 
// Function to scan area using "eyes" servo and return distance
int scanDirection(int angle) {
  eyesServo.write(angle);         // Move eyes servo to the specified angle
  delay(500);                     // Give time to move
  int distance = measureDistance(frontTrigPin, frontEchoPin);
  eyesServo.write(90);            // Return eyes to center
  return distance;
}
 
// Function for gradual steering adjustments
void gradualTurn(int targetAngle) {
  int currentAngle = steeringServo.read();
  if (currentAngle < targetAngle) {
    for (int pos = currentAngle; pos <= targetAngle; pos++) {
      steeringServo.write(pos);
      delay(15);  // Slow down the movement
    }
  } else {
    for (int pos = currentAngle; pos >= targetAngle; pos--) {
      steeringServo.write(pos);
      delay(15);  // Slow down the movement
    }
  }
}
 
// Function to measure distance with HC-SR04 sensor
int measureDistance(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
 
  long duration = pulseIn(echoPin, HIGH, 30000);  // Timeout after 30 ms
  if (duration == 0) {
    return -1;  // Return -1 if no echo is received
  }
  int distance = duration * 0.034 / 2;  // Convert to cm
  return distance;
}

