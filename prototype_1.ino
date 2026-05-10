/*
Prototype 1
//***Still to be edited
//*Delete after implmented/ fixed: Buzzer code, RTC code, errorState code, Serial library funcions fixed

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

RTC:
20: SDA
21: SCL

ADC:
A0: Analog data input

*/
/*
May 9, 2026
Sebastian
Added code from previous labs for UART and ADC.
Split sensorActivate() into 2 functions.
Display now shows the data from the sensor--can be roughly interpreted as the distance from the sensor to the nearest object.
TO-DO:
Add buzzer to ACTIVE behavior. (easy)
Connect LED's (medium--need more pins)
Cycle through data on LCD display (easy/medium)
RTC and logging (medium)
Convert to GPIO (medium/hard)

*/

volatile unsigned char currentState = 0; //0 = OFF, 1 = IDLE, 2 = ACTIVE, 3 = ERROR

//Register bindings for pins 2, 3, 4
//They are all inputs with pull-up
//Pin 2 is PE4
//Pin 3 is PE5
//Pin 4 is PG5
volatile unsigned char* pin_e = (unsigned char*) 0x2C;
volatile unsigned char* ddr_e = (unsigned char*) 0x2D;
volatile unsigned char* port_e = (unsigned char*) 0x2E;
volatile unsigned char* pin_g = (unsigned char*) 0x32;
volatile unsigned char* ddr_g = (unsigned char*) 0x33;
volatile unsigned char* port_g = (unsigned char*) 0x34;

//Used by the UART
#define RDA 0x80
#define TBE 0x20  

//Register bindings for the UART
volatile unsigned char *myUCSR0A = (unsigned char *)0x00C0;
volatile unsigned char *myUCSR0B = (unsigned char *)0x00C1;
volatile unsigned char *myUCSR0C = (unsigned char *)0x00C2;
volatile unsigned int  *myUBRR0  = (unsigned int *) 0x00C4;
volatile unsigned char *myUDR0   = (unsigned char *)0x00C6;
 
//Register bindings for the ADC
volatile unsigned char* my_ADMUX = (unsigned char*) 0x7C;
volatile unsigned char* my_ADCSRB = (unsigned char*) 0x7B;
volatile unsigned char* my_ADCSRA = (unsigned char*) 0x7A;
volatile unsigned int* my_ADC_DATA = (unsigned int*) 0x78;

unsigned char lastState = 255;

#include <LiquidCrystal.h>
const int RS = 10, EN = 9, D4 = 8, D5 = 7, D6 = 6, D7 = 5;
LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);
unsigned long lastRefreshTime = 0;

//ISR-------------
void startISR() {
  Serial.println("innterupt fired");
  currentState = 1;
}

const int TRIGGER = 11, ECHO = 12;
const float breathThreshold = 1000; //A threshold that echoLength must pass to detect a breath. Controls the intended distance from the chest to the sensor.
const float breathTimeout = 2000; //If expectedBreathGap is above this value, the buzzer will activate. (ms)
const float rapidBreathingThreshold = -300; //If expectedBreathGap is below this value, the buzzer will activate. (ms)
const int rapidBreathingTolerance = 3; //The buzzer will activate after this many short breaths.

float echoLength = 0; //Proportional to the distance between the sensor & the chest. This is actually the time it takes the echo from the chest to reach the sensor. (microseconds)
float prevEchoLength = 0; //(microseconds)
float expectedBreathTime = 1200; //The expected time between breaths (ms)
unsigned long previousMicros; //Used by the sensor (microseconds)
unsigned long prevMillis = 0; //The time of the last breath (ms)
float breathGap = 0; //The difference between the duration of the current breath and what it should be.
int breathCount = 0;
int rapidBreathingCounter = 0; //The way I'm detecting breaths is a little sensitive, so I have this variable to reduce false positives until I find a better solution.

unsigned int potentiometerVal = 0;

//sensorActivate() was split into 2 subfunctions
//Because the error state needs to recieve the sensor data without processing it.

//This function will freeze the code if the sensor is not connected.
//Reads data from the sensor and updates echoLength with the value.
void getSensorData() {
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

  delay(50);
}

//Processes the sensor data. Responsible for detecting breaths and changing to ACTIVE and ERROR states.
void processBreathing() {

  //(millis() - prevMillis) is the time since the last breath.
  breathGap = (millis() - prevMillis) - expectedBreathTime;

  if (abs(echoLength - prevEchoLength) > 500) { //If the change in the sensor value was not too rapid (indicating large movements which are not breaths):
    //Do nothing

  } else if (breathGap > breathTimeout && !(currentState == 2)) { //If the breathing is too too slow:
    Serial.println("");
    Serial.println("BEEEEEEEEEEEEEEEEEEEEP: Slow Breathing");
    currentState = 2;

  } else if ((echoLength < breathThreshold) && (prevEchoLength > breathThreshold)) { //If the chest is rising and passes a threshold
    if (breathGap < rapidBreathingThreshold) { //Check for rapid breathing.
      rapidBreathingCounter += 2;
    }

    if (rapidBreathingCounter > rapidBreathingTolerance && !(currentState == 2)) { //If the number of consecutive short breaths is >rapidBreathingTolerance, the buzzer will activate.
      Serial.println("");
      Serial.println("BEEEEEEEEEEEEEEEEEEEEP: Rapid Breathing");
      currentState = 2;

      //In the actual system, we would keep the buzzer running until the RESET button was pressed.
      //Here, the system will resume after breathing returns to normal levels.
      //This assignment is neccessary to do that.
      rapidBreathingCounter = 2;

    } else {
      breathCount++; //Record a breath
      Serial.println("");
      Serial.print("Breath Count: ");
      Serial.println(breathCount);

      //Print expected breath duration and time since the last breath.
      //If leaving the active state, print "Recalculating..." because we have insufficient data.
      if (currentState == 2) {
        Serial.println("Recalculating...");
      } else {
        Serial.print("Expected Time Between Breaths (ms): ");
        Serial.println(expectedBreathTime);
        Serial.print("Actual Time Between Breaths (ms): ");
        Serial.println(millis() - prevMillis);
      }

      currentState = 1;
      if (rapidBreathingCounter > 0) {rapidBreathingCounter--;}
      prevMillis = millis(); //Update/record the time of the breath.
    }
  }
  
  prevEchoLength = echoLength;
}

void setup() {
  pinMode(TRIGGER, OUTPUT); //Set pin 9 as the trigger pin for the sensor.
  pinMode(ECHO, INPUT); //Set pin 10 as the echo pin for the sensor.
  UART_init(9600);
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

  //Initialize the aDC
  adc_init();
}

void loop() {
  potentiometerVal = ( (potentiometerVal * 9) + adc_read() ) / 10; //Stablilizing
  //700 units of range after clamping
  expectedBreathTime = (potentiometerVal > 900) ? (900) : (potentiometerVal); //Clamping
  expectedBreathTime = (potentiometerVal < 200) ? (200) : (potentiometerVal); //Clamping
  expectedBreathTime = 3000 + 2.5 * (expectedBreathTime - 200); //Range: 3000 microseconds to 5035 microseconds
  

  //Reset Button Logic--------------------
  if (currentState != 0) {
    if(!(*pin_g & (1 << 5))) { // pressed LOW
      currentState = 1;

      Serial.println("RESET -> IDLE (DATA CLEARED)");

      while (!(*pin_g & (1 << 5))); //wait release
    }
  }
  //Off Button Logic-----------------------
  if (currentState != 0) {
    if(!(*pin_e & (1 << 5))) { // pressed LOW
      currentState = 0;
      
      while (!(*pin_e & (1 << 5))); //debounce
    }
  }

  if (currentState != lastState) {//WHOLE IF STATEMENT TEMP FOR TEST***
    
    lastState = currentState;
  
    //These activate once: when the state changes
    if (currentState == 0) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("OFF");
      Serial.println("STATE: OFF");
    } else if (currentState == 1) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("IDLE");
      Serial.println("STATE: IDLE");
    } else if (currentState == 2) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ACTIVE");
      Serial.println("STATE: ACTIVE");
    } else if (currentState == 3) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ERROR");
      Serial.println("STATE: ERROR");
    }
  }

  //These activate repeatedtly
  if (currentState != 0) {
    if (currentState == 1) {
      //Idle Behavior
      getSensorData();
      processBreathing();

      if (millis() - lastRefreshTime > 3000) {
        lcd.clear();
        
        lcd.setCursor(0, 0);
        lcd.print("IDLE");

        lcd.setCursor(0, 1);
        lcd.print(echoLength);
        lastRefreshTime = millis();
      }
      
    } else if (currentState == 2) {
      //Active Behavior
      getSensorData();
      processBreathing();

      if (millis() - lastRefreshTime > 3000) {
        lcd.clear();

        lcd.setCursor(0, 0);
        lcd.print("ACTIVE");

        lcd.setCursor(0, 1);
        lcd.print(echoLength);
        lastRefreshTime = millis();
      }

    } else if (currentState == 3) {
      //Error Behavior
      getSensorData();

      if (millis() - lastRefreshTime > 3000) {
        lcd.clear();

        lcd.setCursor(0, 0);
        lcd.print("ERROR");

        lcd.setCursor(0, 1);
        lcd.print(echoLength);
        lastRefreshTime = millis();
      }

    }
  }
}

//Functions for serial I/O
void UART_init(int U0baud) {
 unsigned long FCPU = 16000000;
 unsigned int tbaud;
 tbaud = (FCPU / 16 / U0baud - 1);
 // Same as (FCPU / (16 * U0baud)) - 1;
 *myUCSR0A = 0x20;
 *myUCSR0B = 0x18;
 *myUCSR0C = 0x06;
 *myUBRR0  = tbaud;
}
unsigned char UART_charReady() {
  return *myUCSR0A & RDA;
}
unsigned char UART_getchar() {
  return *myUDR0;
}
void UART_printChar(unsigned char U0pdata) {
  while((*myUCSR0A & TBE)==0);
  *myUDR0 = U0pdata;
}

void adc_init() {
  // setup the A register
 // set bit 7 to 1 to enable the ADC 
 *my_ADCSRA |= 0b10000000;
 // clear bit 5 to 0 to disable the ADC trigger mode
 *my_ADCSRA &= 0b11011111;
 // clear bit 3 to 0 to disable the ADC interrupt 
 *my_ADCSRA &= 0b11110111;
 // clear bit 0-2 to 0 to set prescaler selection to slow reading
 *my_ADCSRA &= 0b11111000;
  // setup the B register
  // clear bit 3 to 0 to reset the channel and gain bits
 *my_ADCSRB &= 0b11110111;
 // clear bit 2-0 to 0 to set free running mode
 *my_ADCSRB &= 0b11111000;
  // setup the MUX Register
 // clear bit 7 to 0 for AVCC analog reference
 *my_ADMUX &= 0b01111111;
  // set bit 6 to 1 for AVCC analog reference
  *my_ADMUX |= 0b01000000;
  // clear bit 5 to 0 for right adjust result
  *my_ADMUX &= 0b11011111;
 // clear bit 4-0 to 0 to reset the channel and gain bits
 *my_ADMUX &= 0b11100000;
}

//Returns the value of the ADC in channel 0.
unsigned int adc_read() { //Only works for channel 0
  // clear the channel selection bits (MUX 4:0)
  *my_ADMUX &= 0b11100000;

  // clear the channel selection bits (MUX 5) hint: it's not in the ADMUX register
  //It's in ADCSRB bit 3
  *my_ADCSRB &= 0b11110111;
 
  // set the channel selection bits for channel 0
  *my_ADMUX &= 0b11100000;
  *my_ADCSRB &= 0b11110111;

  // set bit 6 of ADCSRA to 1 to start a conversion
  *my_ADCSRA |= 0b01000000;

  // wait for the conversion to complete
  while((*my_ADCSRA & 0x40) != 0);

  // return the result in the ADC data register and format the data based on right justification (check the lecture slide)
  unsigned int val = (*my_ADC_DATA & 0x03FF);
  return val;
}
