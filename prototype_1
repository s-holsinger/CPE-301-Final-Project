/*
Prototype 1

Pins:
Buttons:
2: Power on
3: Power off
4: Reset

Display:
5: D7
6: D6
7: D5
8: D4
9: EN
10: RS

Sensor:
11: Trigger
12: Echo

*/

volatile unsigned char currentState = 0; //0 = OFF, 1 = IDLE, 2 = ACTIVE, 3 = ERROR
volatile unsigned char startPressed = 0; //FLAG

volatile unsigned char* pin_e = (unsigned char*) 0x2C;
volatile unsigned char* ddr_e = (unsigned char*) 0x2D;
volatile unsigned char* port_e = (unsigned char*) 0x2E;
volatile unsigned char* pin_g = (unsigned char*) 0x32;
volatile unsigned char* ddr_g = (unsigned char*) 0x33;
volatile unsigned char* port_g = (unsigned char*) 0x34;

unsigned char lastState = 255;

#include <LiquidCrystal.h>
const int RS = 10, EN = 9, D4 = 8, D5 = 7, D6 = 6, D7 = 5;
LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);

//ISR-------------
void startISR() {
  Serial.println("innterupt fired");
  startPressed = 1;
  currentState = 1;
}

const int TRIGGER = 11, ECHO = 12;
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

//This function will freeze the code if the sensor is not connected.
void sensorActivate() {
  //To activate the sensor, we need to send a 10 microsecond pulse via the trigger.
  //This will require a more accurate delay function, which we must reconstruct with a timer.
  digitalWrite(TRIGGER, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIGGER, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGGER, LOW);

  //Getting the sensor value
  while (digitalRead(ECHO) == LOW) {} //Wait while the echo pin is low. The sensor is sending a pulse for detecting an object.
  if (digitalRead(ECHO) == HIGH) { //When the echo pin is high, the sensor is waiting for the echo of the pulse.
    previousMicros = micros(); //Record the start time.
  }
  while (digitalRead(ECHO) != LOW) {} //What until it's low again.
  echoLength = micros() - previousMicros; //Record the total duration. This time is the total time it took for the echo to return to the arduno.
  //We can convert this to a distance by dividing it by the speed of sound. However, I'm not doing that because it's unneccessary.

  //(millis() - prevMillis) is the time since the last breath.
  breathGap = (millis() - prevMillis) - expectedBreathTime;

  if (abs(echoLength - prevEchoLength) > 500) { //If the change in the sensor value was not too rapid (indicating large movements which are not breaths):
    //Do nothing

  } else if (breathGap > breathTimeout && !errorActive) { //If the breathing is too too slow:
    Serial.println("");
    Serial.println("BEEEEEEEEEEEEEEEEEEEEP: Slow Breathing");
    currentState = 2;
    errorActive = true;

  } else if ((echoLength < breathThreshold) && (prevEchoLength > breathThreshold)) { //If the chest is rising and passes a threshold
    if (breathGap < rapidBreathingThreshold) { //Check for rapid breathing.
      rapidBreathingCounter += 2;
    }

    if (rapidBreathingCounter > rapidBreathingTolerance && !errorActive) { //If the number of consecutive short breaths is >rapidBreathingTolerance, the buzzer will activate.
      Serial.println("");
      Serial.println("BEEEEEEEEEEEEEEEEEEEEP: Rapid Breathing");
      currentState = 2;
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

      currentState = 1;
      errorActive = false;
      if (rapidBreathingCounter > 0) {rapidBreathingCounter--;}
      prevMillis = millis(); //Update/record the time of the breath.
    }
  }
  
  prevEchoLength = echoLength;

  delay(50);
}

void setup() {
  pinMode(TRIGGER, OUTPUT); //Set pin 9 as the trigger pin for the sensor.
  pinMode(ECHO, INPUT); //Set pin 10 as the echo pin for the sensor.
  Serial.begin(9600);
//Start Button ---------------------------
  *ddr_e &= ~(1 << 4);  //PIN 2 input
  *port_e |= (1 << 4); //Enable pull up resistor on pin 2
  attachInterrupt(digitalPinToInterrupt(2), startISR, FALLING);  //Interrupt on falling edge (button press)
//----------------------------------------
//Off Button -----------------------------
  *ddr_e &= ~(1 << 5);  //PIN 3 input
  *port_e |= (1 << 5);  //pull up
//----------------------------------------
//Reset Button ---------------------------
  *ddr_g &= ~(1 << 5);  //PIN 4 input
  *port_g |= (1 << 5);  //pull up
//----------------------------------------
  currentState = 0;
  lastState = 255;
  lcd.begin(16, 2);

  Serial.println("System started in OFF state"); //TO TEST***
}

void loop() {

  //Reset Button Logic--------------------
  if (currentState != 0) {
    if(!(*pin_g & (1 << 5))) { // pressed LOW
      currentState = 1;
      startPressed = 0;

      Serial.println("RESET -> IDLE (DATA CLEARED)");

      while (!(*pin_g & (1 << 5))); //wait release
    }
  }
  //Off Button Logic-----------------------
  if (currentState != 0) {
    if(!(*pin_e & (1 << 5))) { // pressed LOW
      currentState = 0;
      startPressed = 0;
      
      while (!(*pin_e & (1 << 5))); //debounce
    }
  }

  if (currentState != lastState) {//WHOLE IF STATEMENT TEMP FOR TEST***
    
    lastState = currentState;
  
    if (currentState == 0) {
      Serial.println("STATE: OFF");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("OFF");
    } else if (currentState == 1) {
      Serial.println("STATE: IDLE");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("IDLE");
    } else if (currentState == 2) {
      Serial.println("STATE: ACTIVE");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ACTIVE");
    } else if (currentState == 3) {
      Serial.println("STATE: ERROR");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ERROR");
    }
  }

  if (currentState != 0) {
    if (currentState == 1) {
      //Idle Behavior
      sensorActivate();
    } else if (currentState == 2) {
      //Active Behavior
      sensorActivate();
    } else if (currentState == 3) {
      //Error Behavior
    }
  }
}
