#include <Servo.h>
#include <LiquidCrystal.h>

// Pin Definitions
const int relayPin = 7;        // Relay control pin for the pump
const int waterLevelPin = A0;  // Water level sensor connected to analog pin
const int lidServoPin = 9;     // Servo motor for the lid
const int pipeServoPin = 10;   // Servo motor for the pipe angle

// LCD Pin Definitions (RS, E, D4, D5, D6, D7)
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

Servo lidServo;  // Servo for the lid
Servo pipeServo; // Servo for the pipe

int waterLevel = 0;        // Water level percentage
String systemState = "Idle"; // Current system state
bool pumpRunning = false;  // Pump status
bool systemStarted = false; // Tracks whether the system has started

void setup() {
  Serial.begin(38400);
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW); // Start with the pump OFF

  // Initialize Servos
  lidServo.attach(lidServoPin);
  pipeServo.attach(pipeServoPin);
  lidServo.write(165);  // Pipe starts in optimal position
  pipeServo.write(0);  // Pipe starts in optimal position

  // Initialize LCD
  lcd.begin(16, 2);
  lcd.print("System Ready");
  lcd.setCursor(0, 1);
  lcd.print("Status: Idle");
}

void loop() {
  // Read water level
  int waterLevelRaw = analogRead(waterLevelPin);
  waterLevel = map(waterLevelRaw, 0, 1023, 0, 100); // Scale to 0-100%

  // Send water level data to MATLAB GUI
  Serial.print("Water Level:");
  Serial.println(waterLevel);

  // Update LCD with water level
  if (systemStarted) {
    lcd.setCursor(0, 0);
    lcd.print("Water Level: ");
    lcd.print(waterLevel);
    lcd.print("%  "); // Clear trailing chars

    // Control the lid based on water level
    if (waterLevel < 40) {
      lidServo.write(165); // Open the lid
      systemState = "Low Water";
      lcd.setCursor(0, 1);
      lcd.print("Status: Low Water");
      if (pumpRunning) {
        digitalWrite(relayPin, LOW); // Stop the pump
        pumpRunning = false;
      }
    } else if (pumpRunning) {
      lidServo.write(200); // Close the lid
      systemState = "Filtering";
      lcd.setCursor(0, 1);
      lcd.print("Status: Filtering ");
    }
  }

  // Listen for GUI commands
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();

    if (command == "START") {
      if (waterLevel >= 40) {
        systemStarted = true; // Mark system as started
        digitalWrite(relayPin, HIGH); // Start the pump
        pumpRunning = true;
        systemState = "Filtering";
        lcd.setCursor(0, 1);
        lcd.print("Status: Filtering ");
        Serial.println("PUMP:ON");
      } else {
        Serial.println("ERROR:Low Water");
      }
    } else if (command == "STOP") {
      digitalWrite(relayPin, LOW); // Stop the pump
      pumpRunning = false;
      systemState = "Stopped";
      lcd.setCursor(0, 1);
      lcd.print("Status: Stopped   ");
      Serial.println("PUMP:OFF");
    } else if (command == "CHANGE") {
      togglePipeServo(); // Change pipe direction
      Serial.println("PIPE:CHANGED");
    }
  }

  delay(500); // Update every 500ms
}

// Function to toggle the pipe direction to 270 degrees
void togglePipeServo() {
  static int direction = 0; // Initial position 
  direction = (direction == 0) ? 180 : 0; // Toggle between 0 and 180 degrees
  pipeServo.write(direction); // Set the servo to the new position

  // Update LCD to show the new direction
  lcd.setCursor(0, 1);
  lcd.print("Pipe Direction: ");
  lcd.print(direction); // Display the current angle
  lcd.print("     "); // Clear any characters
}