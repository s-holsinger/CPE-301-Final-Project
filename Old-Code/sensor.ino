//Sensor code by Sebastian Holsinger
//These settings assume that the chest is about ~4-5 inches from the sensor (this can be configured with the breathThreshold variable).
//The way I'm detecting breaths is a bit sensitive. I would reccommend using a large, flat object to simulate breathing.
//If the sensor detects a large period without breathing, it will display an error, which would activate the buzzer in the full device.
//If the sensor (HC-SR04) detects a sequence of short breaths, it will display an error.
//All you need to do is connect the pins to the sensor and compile.
//VCC -> 5V
//GRD -> GRD
//Trig -> Pin 9
//Echo -> Pin 10

void setup() {
  pinMode(9, OUTPUT); //Set pin 9 as the trigger pin for the sensor.
  pinMode(10, INPUT); //Set pin 10 as the echo pin for the sensor.
  Serial.begin(9600);
}

const float breathThreshold = 1000; //A threshold that echoLength must pass to detect a breath. Controls the intended distance from the chest to the sensor.
const float breathTimeout = 2000; //If expectedBreathGap is above this value, the buzzer will activate. (ms)
const float rapidBreathingThreshold = -300; //If expectedBreathGap is below this value, the buzzer will activate. (ms)
const unsigned int expectedBreathDrift = 20; //The expected respiratory rate is allowed to drift over time. Larger values for expectedBreathDrift will make the expected rate more stable.
const int rapidBreathingTolerance = 3; //The buzzer will activate after this many short breaths.

float echoLength = 0; //Proportional to the distance between the sensor & the chest. This is actually the time it takes the echo from the chest to reach the sensor. (microseconds)
float prevEchoLength = 0; //(microseconds)
float expectedBreathTime = 1200; //The expected time between breaths (ms)
unsigned long previousMicros; //Used by the sensor (microseconds)
unsigned long prevMillis = 0; //The time of the last breath (ms)
float breathGap = 0; //The difference between the duration of the current breath and what it should be.
int breathCount = 0;
int rapidBreathingCounter = 0; //The way I'm detecting breaths is a little sensitive, so I have this variable to reduce false positives until I find a better solution.
bool errorActive = false;

void loop() {
  //To activate the sensor, we need to send a 10 microsecond pulse via the trigger.
  //This will require a more accurate delay function, which we must reconstruct with a timer.
  digitalWrite(9, LOW);
  delayMicroseconds(2);
  digitalWrite(9, HIGH);
  delayMicroseconds(10);
  digitalWrite(9, LOW);

  //Getting the sensor value
  while (digitalRead(10) == LOW) {} //Wait while the echo pin is low. The sensor is sending a pulse for detecting an object.
  if (digitalRead(10) == HIGH) { //When the echo pin is high, the sensor is waiting for the echo of the pulse.
    previousMicros = micros(); //Record the start time.
  }
  while (digitalRead(10) != LOW) {} //What until it's low again.
  echoLength = micros() - previousMicros; //Record the total duration. This time is the total time it took for the echo to return to the arduno.
  //We can convert this to a distance by dividing it by the speed of sound. However, I'm not doing that because it's unneccessary.

  //(millis() - prevMillis) is the time since the last breath.
  breathGap = (millis() - prevMillis) - expectedBreathTime;

  if (abs(echoLength - prevEchoLength) > 500) { //If the change in the sensor value was not too rapid (indicating large movements which are not breaths):
    //Do nothing

  } else if (breathGap > breathTimeout && !errorActive) { //If the breathing is too too slow:
    Serial.println("");
    Serial.println("BEEEEEEEEEEEEEEEEEEEEP: Slow Breathing");
    errorActive = true;

  } else if ((echoLength < breathThreshold) && (prevEchoLength > breathThreshold)) { //If the chest is rising and passes a threshold
    if (breathGap < rapidBreathingThreshold) { //Check for rapid breathing.
      rapidBreathingCounter += 2;
    }

    if (rapidBreathingCounter > rapidBreathingTolerance && !errorActive) { //If the number of consecutive short breaths is >rapidBreathingTolerance, the buzzer will activate.
      Serial.println("");
      Serial.println("BEEEEEEEEEEEEEEEEEEEEP: Rapid Breathing");
      errorActive = true;

      //In the actual system, we would keep the buzzer running until the RESET button was pressed.
      //Here, the system will resume after breathing returns to normal levels.
      //This assignment is neccessary to do that.
      rapidBreathingCounter = 2;

    } else {
      breathCount++; //Record a breath
      Serial.println("");
      Serial.print("Breath Count: ");
      Serial.println(breathCount);

      //Recalculate the expected breath duration.
      if (errorActive) {
        Serial.println("Recalculating...");
      } else {
        expectedBreathTime = (expectedBreathTime * (expectedBreathDrift - 1) + (millis() - prevMillis) ) / expectedBreathDrift;
        Serial.print("Expected Time Between Breaths (ms): ");
        Serial.println(expectedBreathTime);
        Serial.print("Actual Time Between Breaths (ms): ");
        Serial.println(millis() - prevMillis);
      }

      errorActive = false;
      if (rapidBreathingCounter > 0) {rapidBreathingCounter--;}
      prevMillis = millis(); //Update/record the time of the breath.
    }
  }
  
  prevEchoLength = echoLength;

  delay(50);
}
