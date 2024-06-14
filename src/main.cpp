#include <SD.h>
#include <SPI.h>

const int solenoidPin = 27; // Pin connected to solenoid
const int endstopPin = 34;  // Pin connected to endstop (must be interrupt-capable pin)
const int chipSelect = 5;   // SD card CS pin

volatile int endstopCounter = 0;
unsigned long lastCycleEnd = 0; // When (in millis) did the last test cycle end
unsigned int lineNumber = 1;    // Initialize line number

const unsigned long solenoidFrequency = 30000;  // pwm frequency for solenoid (Hz)
const unsigned long solenoidPull = 255;         // pwm value for solenoid on (initial pull, max 255)
const unsigned long solenoidHold = 100;         // pwm value for solenoid hold (holding while on, max 255)
const unsigned long solenoidPullDuration = 100; // duration solenoid operates at pull setting

const unsigned long maxOnDuration = 2000; // Maximum duration solenoid can stay on in milliseconds
const unsigned long maxInterval = 3000;   // Maximum interval between cycles in milliseconds
const unsigned long minInterval = 500;    // Minimum interval between cycles in milliseconds

unsigned long timeBetweenTests = random(minInterval, maxInterval); // Generate a random interval

void endstopISR()
{
  endstopCounter++; // Increment the counter in the ISR
}

void logResult(String result)
{
  File dataFile = SD.open("/text.txt", FILE_APPEND);
  if (dataFile)
  {
    dataFile.print(lineNumber); // Print the line number
    dataFile.print(": ");
    String colorCode;
    if (result.equals("PASS"))
      colorCode = "\x1B[92m"; // Green
    else
      colorCode = "\x1B[91m";                         // Red
    dataFile.println(colorCode + result + "\x1B[0m"); // Reset to default color
    dataFile.close();

    // Log to serial port
    Serial.print(lineNumber); // Print the line number to serial monitor
    Serial.print(": ");
    if (result.equals("PASS"))
      Serial.print("\x1B[92m"); // Green
    else
      Serial.print("\x1B[91m");         // Red
    Serial.println(result + "\x1B[0m"); // Reset to default color
    lineNumber++;                       // Increment the line number for next entry
  }
  else
  {
    Serial.println("Error opening datalog.txt");
  }
}

void setup()
{
  Serial.begin(115200);
  analogWriteFrequency(solenoidFrequency);
  pinMode(solenoidPin, OUTPUT);
  pinMode(endstopPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(endstopPin), endstopISR, FALLING); // Attach interrupt
  if (!SD.begin(chipSelect))
  {
    Serial.println("Card failed, or not present");
    while (1)
      ;
  }
  Serial.println("Card initialized.");
  randomSeed(analogRead(0)); // Seed the random generator with a noise reading

  Serial.println("Solenoid initialization test. Solenoid On for 2 second");
  analogWrite(solenoidPin, solenoidPull);
  delay(2000);
  Serial.println("Solenoid initialization test. Solenoid Off for 2 second");
  analogWrite(solenoidPin, 0);
  delay(2000);
  Serial.println("Solenoid initialization test. Solenoid On for 1 second");
  analogWrite(solenoidPin, solenoidPull);
  delay(1000);
  Serial.println("Solenoid initialization test. Solenoid Off for 1 second");
  analogWrite(solenoidPin, 0);
  delay(1000);
}

void loop()
{
  unsigned long currentMillis = millis();

  if (currentMillis - lastCycleEnd >= timeBetweenTests)
  {

    unsigned long onDuration = random(maxOnDuration); // Generate a random on duration

    Serial.println("Solenoid ON (" + String(onDuration) + " milliseconds)");
    analogWrite(solenoidPin, solenoidPull);
    delay(solenoidPullDuration);
    analogWrite(solenoidPin, solenoidHold);
    delay(onDuration);
    analogWrite(solenoidPin, 0);

    Serial.println("Solenoid OFF");

    unsigned long startTime = millis();
    Serial.println(String(endstopCounter));
    while (millis() - startTime < 500)
    { // 500 ms timeout for detecting endstop
      if (endstopCounter >= 2)
      { // Check if both pulses are detected
        break;
      }
    }

    if (endstopCounter >= 2)
    {
      logResult("PASS");
    }
    else
    {
      logResult("FAIL");
    }

    endstopCounter = 0;                                  // Reset the endstop counter
    timeBetweenTests = random(minInterval, maxInterval); // Generate a new random interval
    lastCycleEnd = currentMillis;                        // Reset time when test ended to now
    Serial.println("Cycle completed. Waiting for " + String(timeBetweenTests) + " milliseconds.\n");
  }
}