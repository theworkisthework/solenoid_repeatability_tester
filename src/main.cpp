#include <SD.h>
#include <SPI.h>

const int solenoidPin = 27;  // Pin connected to solenoid
const int endstopPin = 34;   // Pin connected to endstop (must be interrupt-capable pin)
const int chipSelect = 5;    // SD card CS pin

volatile int endstopCounter = 0;
unsigned long lastTrigger = 0;
unsigned int lineNumber = 1; // Initialize line number

const unsigned long maxOnDuration = 2000; // Maximum duration solenoid can stay on in milliseconds
const unsigned long maxInterval = 3000;   // Maximum interval between cycles in milliseconds

void endstopISR() {
  endstopCounter++;  // Increment the counter in the ISR
}

void logResult(String result) {
  File dataFile = SD.open("/text.txt", FILE_APPEND);
  if (dataFile) {
    dataFile.print(lineNumber);  // Print the line number
    dataFile.print(": ");
    dataFile.println(result);
    dataFile.close();
    Serial.print(lineNumber);    // Print the line number to serial monitor
    Serial.print(": ");
    Serial.println(result);
    lineNumber++;  // Increment the line number for next entry
  } else {
    Serial.println("Error opening datalog.txt");
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(solenoidPin, OUTPUT);
  pinMode(endstopPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(endstopPin), endstopISR, FALLING);  // Attach interrupt
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    while (1);
  }
  Serial.println("Card initialized.");
  randomSeed(analogRead(0)); // Seed the random generator with a noise reading
}

void loop() {
  unsigned long currentMillis = millis();
  unsigned long randomInterval = random(maxInterval); // Generate a random interval

  if (currentMillis - lastTrigger >= randomInterval) {
    lastTrigger = currentMillis;
    endstopCounter = 0;  // Reset the endstop counter

    unsigned long onDuration = random(maxOnDuration); // Generate a random on duration
    Serial.println("Triggering solenoid ON for " + String(onDuration) + " milliseconds");
    digitalWrite(solenoidPin, HIGH);
    delay(onDuration);
    digitalWrite(solenoidPin, LOW);
    Serial.println("Solenoid OFF");

    unsigned long startTime = millis();
    while (millis() - startTime < 500) { // 500 ms timeout for detecting endstop
      if (endstopCounter >= 2) { // Check if both pulses are detected
        break;
      }
    }

    if (endstopCounter >= 2) {
      logResult("Success");
    } else {
      logResult("Failed");
    }

    Serial.println("Cycle completed. Waiting for next trigger.");
  }
}
