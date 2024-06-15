#include <SD.h>
#include <SPI.h>

const int solenoidPin = 27; // Pin connected to solenoid
const int endstopPin = 34;  // Pin connected to endstop (must be interrupt-capable pin)
const int chipSelect = 5;   // SD card CS pin

volatile int endstopCounter = 0;
unsigned long lastCycleEnd = 0; // When (in millis) did the last test cycle end
unsigned int testCount = 1;     // Initialize line number
boolean detectedSolenoidOn = 0;
boolean detectedSolenoidOff = 0;
unsigned long startTime;

const unsigned long solenoidFrequency = 30000;  // pwm frequency for solenoid (Hz)
const unsigned long solenoidPull = 255;         // pwm value for solenoid on (initial pull, max 255)
const unsigned long solenoidHold = 100;         // pwm value for solenoid hold (holding while on, max 255)
const unsigned long solenoidPullDuration = 100; // duration solenoid operates at pull setting

// Should we even wait for some time to say we have detected the trigger?
// Why not just wait until we have detected the trigger and timeout? We do that, but we also need to say
// that if after a certain length of time, we haven't detected the trigger, then we can say the activation
// or deactivation has failed. It might be slow or lazy in which case we should consider that a fail.
const unsigned long solenoidActivationDectectionWindow = 100;   // Time we wait before checking for activation
const unsigned long solenoidDeactivationDectectionWindow = 200; // Time we wait before checking for deactivation (note: this takes longer than activation)

const unsigned long maxOnDuration = 2500; // Maximum duration solenoid can stay on in milliseconds
const unsigned long maxInterval = 1500;   // Maximum interval between cycles in milliseconds
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
    dataFile.print("Test " + String(testCount)); // Print the line number
    dataFile.print(": ");
    String colorCode;
    if (result.equals("PASS"))
      colorCode = "\x1B[92m"; // Green
    else
      colorCode = "\x1B[91m";                         // Red
    dataFile.println(colorCode + result + "\x1B[0m"); // Reset to default color
    dataFile.close();

    // Log to serial port
    Serial.print("Test " + String(testCount)); // Print the line number to serial monitor
    Serial.print(": ");
    if (result.equals("PASS"))
      Serial.print("\x1B[92m"); // Green
    else
      Serial.print("\x1B[91m");         // Red
    Serial.println(result + "\x1B[0m"); // Reset to default color
    testCount++;                        // Increment the line number for next entry
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
  if (millis() - lastCycleEnd >= timeBetweenTests)
  {

    unsigned long onDuration = random(maxOnDuration); // Generate a random on duration

    Serial.println("Solenoid ON (" + String(onDuration) + " milliseconds)");
    endstopCounter = 0; // Reset end stop counter
    // Solenoid on at pull strength for pull duration
    analogWrite(solenoidPin, solenoidPull);

    // Check that endstop was tirggered at solenoid on
    startTime = millis();
    while (millis() - startTime < solenoidActivationDectectionWindow)
    { // 500 ms timeout for detecting endstop
      if (endstopCounter >= 1)
      { // Check if both pulses are detected
        Serial.println("Detected solenoid activation");
        detectedSolenoidOn = true;
        break;
      }
    }
    endstopCounter = 0; // Reset end stop counter
    // Wait for the pull duration (Should we subtract the detection time? It might not always be the full lenght of time if detection breaks out of the while loop)
    if (solenoidPullDuration - (millis() - startTime) > 0)
    {
      delay(solenoidPullDuration - (millis() - startTime));
      // Solenoid on at pull strength for pull duration less however long we have already waited for endstop detection
      // If this duration is less than 1, we've already waited longer than the pull duration so don't wait any longer
    }

    // Solenoid on at hold strength
    analogWrite(solenoidPin, solenoidHold);
    delay(onDuration);

    endstopCounter = 0; // Reset the endstop counter just to clear any anaomalous readings
    // Solenoid off
    analogWrite(solenoidPin, 0);
    Serial.println("Solenoid OFF");
    startTime = millis();
    while (millis() - startTime < solenoidDeactivationDectectionWindow)
    { // 500 ms timeout for detecting endstop
      if (endstopCounter >= 1)
      { // Check if both pulses are detected
        Serial.println("Detected solenoid de-activation");
        detectedSolenoidOff = true;
        break;
      }
    }
    endstopCounter = 0; // Probably redundant, but just for good measure, clear the endstop counter again

    Serial.println("Detected: Solenoid ON: " + String(detectedSolenoidOn) + " Solenoid OFF: " + String(detectedSolenoidOff) + "");

    if (detectedSolenoidOn & detectedSolenoidOff)
    {
      logResult("PASS");
    }
    else
    {
      logResult("FAIL");
    }

    detectedSolenoidOn = false; // Reset solenoid detection states
    detectedSolenoidOff = false;
    // endstopCounter = 0;                                  // Reset the endstop counter
    timeBetweenTests = random(minInterval, maxInterval); // Generate a new random interval
    lastCycleEnd = millis();                             // Reset time when test ended to now
    Serial.println("Test complete. Waiting for " + String(timeBetweenTests) + " milliseconds.\n");
  }
}