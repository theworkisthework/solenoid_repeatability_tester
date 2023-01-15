#include <Arduino.h>
#include <SPI.h>
#include <SD.h>

File sdCardFile;

// create a variable to store the time when the last reading was taken
unsigned long lastTime = 0;

// variable to store the count of the number of times the solenoid has been triggered
int triggerCount = 0;

int solenoidPin = 27; // Solenoid pin
int endStopPin = 34;  // End stop pin

// Triger is how often we can trigger the solenoid
int minTriggerDelay = 250;  // Minimum delay between triggers
int maxTriggerDelay = 1000; // Maximum delay between triggers

int solenoidHoldTime = 500; // How long we hold the solenoid for

// Sense time is how long we wait for the end stop to be triggered (more than maxSenseTime and we assume it's a failed activation)
int minSenseTime = 100; // Minimum time between activating the solenoid and reading the end stop
int maxSenseTime = 500; // Maximum time between activating the solenoid and reading the end stop

// Consequitive triggers is the number of times we can trigger the solenoid in a row
int consequitiveTriggers = 0;

void setup()
{
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  Serial.print("Initializing SD card...");

  if (!SD.begin(5))
  {
    Serial.println("initialization failed!");
    while (1)
      ;
  }
  Serial.println("initialization done.");

  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  sdCardFile = SD.open("test.txt", FILE_WRITE);

  // if the file opened okay, write to it:
  if (sdCardFile)
  {
    Serial.print("Starting Test");
    sdCardFile.println("======= Starting Test =======");
    // close the file:
    sdCardFile.close();
    Serial.println("done.");
  }
  else
  {
    // if the file didn't open, print an error:
    Serial.println("error opening test.txt");

    // initalize the lastTime variable
    lastTime = millis();

    // initialize the solenoid pin as an output:
    pinMode(solenoidPin, OUTPUT);
    // set the solenoid pin to LOW:
    digitalWrite(solenoidPin, LOW);

    // initialize the end stop pin as an input:
    pinMode(endStopPin, INPUT);
  }
}

void loop()
{
  // Trigger the solenoid
  Serial.println("Triggering Solenoid");
  sdCardFile = SD.open("test.txt", FILE_WRITE);
  // sdCardFile.println("Triggering Solenoid" + String(millis() - lastTime));
  // Write the above to serial
  // Serial.println("Triggering Solenoid" + String(millis() - lastTime));
  // Trigger the solenoid
  digitalWrite(solenoidPin, HIGH);
  // Wait minSenseTime
  delay(minSenseTime);
  // update lastTime
  lastTime = millis();
  boolean endStopTriggered = false;
  // Keep checking the end stop until it's triggered or maxSenseTime is reached, if it's triggered, set endStopTriggered to true
  while (millis() - lastTime < maxSenseTime)
  {
    if (digitalRead(endStopPin) == HIGH)
    {
      endStopTriggered = true;
      break;
    }
  }
  // If the end stop was triggered, write a pass result to the sd card and serial else write a fail result to the sd card and serial
  if (endStopTriggered)
  {
    // increment the trigger count
    triggerCount++;
    // increnent consequitiveTriggers
    consequitiveTriggers++;

    Serial.println(String(triggerCount) + ": PASSED in " + (millis() - lastTime) + "ms. Consequitive Triggers: " + String(consequitiveTriggers));
    sdCardFile.println(String(triggerCount) + ": PASSED in " + (millis() - lastTime) + "ms. Consequitive Triggers: " + String(consequitiveTriggers));
  }
  else
  {
    Serial.println(String(triggerCount) + ": FAILED in " + (millis() - lastTime) + "ms. Fail after " + String(consequitiveTriggers) + " Consequitive Triggers");
    sdCardFile.println(String(triggerCount) + ": FAILED in " + (millis() - lastTime) + "ms. Fail after " + String(consequitiveTriggers) + " Consequitive Triggers");
    consequitiveTriggers = 0;
  }
  // Close the file
  sdCardFile.close();
  // wait until hold time is reached then reset the solenoid
  delay(solenoidHoldTime);
  // reset the solenoid
  digitalWrite(solenoidPin, LOW);
  // reset endStopTriggered
  endStopTriggered = false;
  // Wait a random amount of time between minTriggerDelay and maxTriggerDelay
  delay(random(minTriggerDelay, maxTriggerDelay));
}