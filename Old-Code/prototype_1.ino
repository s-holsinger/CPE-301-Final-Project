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

May 10, 2026
Added buzzer and LED code.
TO-DO:
RTC and logging (medium)
Convert to GPIO (medium/hard)

May 10, 2026
Added RTC and Logging
Updated Display
TO-DO:
Convert to GPIO (medium/hard)

May 11, 2026
Debugging--tweaked some values and moved some things around.
This code was written yesterday and used for the video.
Remaining tasks are trivial.
TO-DO:
Convert to GPIO (easy/medium)

May 11, 2026
Converted serial functions to GPIO.
TO-DO
Convert delay() to GPIO (easy)

*/




#include <RTClib.h>
#include <math.h>
RTC_DS1307 rtc;
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday",
"Friday", "Saturday"};
unsigned long previousTime = 0;
const unsigned long interval = 60000;
char buffer[12];

volatile unsigned char currentState = 0; //0 = OFF, 1 = IDLE, 2 = ACTIVE, 3 = ERROR

//Register bindings for pins 2, 3, 4
//They are all inputs with pull-up
//Pin 2 is PE4 (On Button)
//Pin 3 is PE5 (Off Button)
//Pin 4 is PG5 (Reset Button)
//Pin 11 is PB5 (Trigger)
//Pin 12 is PB6 (Echo)
//Pin 13 is PB7 (Buzzer)
//Pin 22 is PA0 (LED)
//Pin 24 is PA4 (LED)
//Pin 26 is PA6 (LED)
//Pin 28 is PA8 (LED)
volatile unsigned char* pin_e = (unsigned char*) 0x2C;
volatile unsigned char* ddr_e = (unsigned char*) 0x2D;
volatile unsigned char* port_e = (unsigned char*) 0x2E;
volatile unsigned char* pin_g = (unsigned char*) 0x32;
volatile unsigned char* ddr_g = (unsigned char*) 0x33;
volatile unsigned char* port_g = (unsigned char*) 0x34;
volatile unsigned char* port_b = (unsigned char*) 0x25;
volatile unsigned char* ddr_b = (unsigned char*) 0x24;
volatile unsigned char* pin_b = (unsigned char*) 0x23;
volatile unsigned char* ddr_a = (unsigned char*) 0x21;
volatile unsigned char* port_a = (unsigned char*) 0x22;

#include <LiquidCrystal.h>
const int RS = 10, EN = 9, D4 = 8, D5 = 7, D6 = 6, D7 = 5; //Pins for LCD display
LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);
unsigned long lastRefreshTime = 0;

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

// Timer 1 Pointers
volatile unsigned char *myTCCR1A = (unsigned char*) 0x80;
volatile unsigned char *myTCCR1B = (unsigned char*) 0x81;
volatile unsigned char *myTCCR1C = (unsigned char*) 0x82;
volatile unsigned char *myTIMSK1 = (unsigned char*) 0x6F;
volatile unsigned char *myTIFR1 = (unsigned char*) 0x36;
volatile unsigned int  *myTCNT1  = (unsigned  int*) 0x84;

unsigned char lastState = 255;
unsigned int errorCounter = 0; //For detecting out-of-range sensor values
unsigned int errorCounterAlt = 0; //For detecting constant sensor data
unsigned int potentiometerVal = 0;
unsigned long potentiometerDisplayTime;

unsigned long loggingTime = 0;
int logBreathCount = 0;


//-------------------
//UART (Serial I/O)
  //From previous labs
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
  void UART_printInt(int num) {
    int numDigitCount = floor(log10((double) num));
    for (int i = numDigitCount; i >= 0; i--) {
      UART_printChar( ((int) fmod(num, pow(10, i + 1))) / ((int) pow(10, i)) + 0x30);
    }
  }
  void UART_printFloat(float num) {
    UART_printInt((int) num);
    UART_printChar('.');
    UART_printChar( ((int) (fmod(num, 1.0) / 0.1)) + 0x30);
    UART_printChar( ((int) (fmod(num + 0.01, 0.1) / 0.01)) + 0x30);
    
  }
  //Should send string
  //Cydney's
  void UART_sendString(const char *str){
    while (*str){
      UART_printChar(*str);
      str++;
    }
  }

  //Also Cydney's
  void UART_printTimestamp() {

    previousTime = millis();

    DateTime now = rtc.now();
    //for the Hour
    UART_printInt(now.hour());
    UART_printChar(':');
    
    //for the Minute
    UART_printInt(now.minute());
    UART_printChar(':');
    
    //for the Second
    UART_printInt(now.second());

  }

//-------------------

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

//Start Button ISR-------------
  void startISR() {
    if (currentState != 3) {
      currentState = 1;
      potentiometerDisplayTime = millis();
    }
  }
//-------------------

//-------------------
//SENSOR
  //Sensor settings that must be tuned and are constant.
  //Used by the getSensorData() and processBreathing() functions
  const int TRIGGER = 11, ECHO = 12;
  const float breathThreshold = 1000; //A threshold that echoLength must pass to detect a breath. Controls the intended distance from the chest to the sensor.
  const float breathTimeout = 2000; //If expectedBreathGap is above this value, the buzzer will activate. (ms)
  const float rapidBreathingThreshold = -300; //If expectedBreathGap is below this value, the buzzer will activate. (ms)
  const int rapidBreathingTolerance = 3; //The buzzer will activate after this many short breaths.

  //Variables used by the getSensorData() and processBreathing() functions
  float echoLength = 0; //Proportional to the distance between the sensor & the chest. This is actually the time it takes the echo from the chest to reach the sensor. (microseconds)
  float prevEchoLength = 0; //(microseconds)
  float expectedBreathTime = 1200; //The expected time between breaths (ms)
  unsigned long previousMicros; //Used by the sensor (microseconds)
  unsigned long prevMillis = 0; //The time of the last breath (ms)
  float breathGap = 0; //The difference between the duration of the current breath and what it should be.
  int breathCount = 0;
  int rapidBreathingCounter = 0; //The way I'm detecting breaths is a little sensitive, so I have this variable to reduce false positives until I find a better solution.

  //sensorActivate() was split into 2 subfunctions
  //Because the error state needs to recieve the sensor data without processing it.

  //This function will freeze the code if the sensor is not connected.
  //Reads data from the sensor and updates echoLength with the value.
  void getSensorData() {
    //To activate the sensor, we need to send a 10 microsecond pulse via the trigger.
    //This will require a more accurate delay function, which we must reconstruct with a timer.
    // digitalWrite(TRIGGER, LOW);
    *port_b &= !(1 << 5);
    delayMicroseconds(2);
    // digitalWrite(TRIGGER, HIGH);
    *port_b |= (1 << 5);
    delayMicroseconds(10);
    // digitalWrite(TRIGGER, LOW);
    *port_b &= !(1 << 5);

    //Getting the sensor value
    // while (digitalRead(ECHO) == LOW) {} //Wait while the echo pin is low. The sensor is sending a pulse for detecting an object.
    while (!(*pin_b & (1 << 6))) {}
    // if (digitalRead(ECHO) == HIGH) { //When the echo pin is high, the sensor is waiting for the echo of the pulse.
    if (*pin_b & (1 << 6)) {
      previousMicros = micros(); //Record the start time.
    }
    // while (digitalRead(ECHO) != LOW) {} //What until it's low again.
    while (*pin_b & (1 << 6)) {}
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

      currentState = 2;

    } else if ((echoLength < breathThreshold) && (prevEchoLength > breathThreshold)) { //If the chest is rising and passes a threshold
      if (breathGap < rapidBreathingThreshold) { //Check for rapid breathing.
        if (rapidBreathingCounter < (rapidBreathingTolerance + 3)) {
          rapidBreathingCounter += 2;
        }
      }

      if (rapidBreathingCounter > rapidBreathingTolerance && !(currentState == 2)) { //If the number of consecutive short breaths is >rapidBreathingTolerance, the buzzer will activate.
        
        currentState = 2;

        rapidBreathingCounter = 2;

      } else {
        breathCount++; //Record a breath
        UART_printTimestamp();
        UART_sendString(" - Breath Count: ");
        UART_printInt(breathCount);
        UART_sendString("\r\n");

        //If leaving the active state, print "Recalculating..." because we have insufficient data.
        if (currentState == 2) {
          UART_sendString("Recalculating...");
          UART_sendString("\r\n");
        }

        if (rapidBreathingCounter < 2) {currentState = 1;}
        if (rapidBreathingCounter > 0) {rapidBreathingCounter--;}
        prevMillis = millis(); //Update/record the time of the breath.
      }
    }
    
    prevEchoLength = echoLength;
  }
//-------------------

//-------------------
//BUZZER
  bool buzzerRunning = false;
  unsigned int buzzerFreqency = 15296;
  //Buzzer TIMER OVERFLOW ISR
  ISR(TIMER1_OVF_vect)
  {
    // Stop the Timer
    *myTCCR1B &= 0b11111000;
    // Load the Count
    *myTCNT1 =  (unsigned int) (65535 -  (unsigned long) (buzzerFreqency));
    // Start the Timer
    *myTCCR1B |= 0x01;
    // If buzzer running
    if(buzzerRunning)
    {
      // XOR to toggle PB7
      *port_b ^= 0x80;
    }
  }

  // Buzzer timer (channel 1) initialization function
  void init_timer_one() {
    // setup the timer control registers
    *myTCCR1A= 0x00;
    *myTCCR1B= 0X00;
    *myTCCR1C= 0x00;
    
    // reset the TOV flag
    *myTIFR1 |= 0x01;
    
    // enable the TOV interrupt
    *myTIMSK1 |= 0x01;
  }

  void start_buzzer() {
    if (!buzzerRunning) {
      //Start the timer
      *myTCCR1B |= 0x01;
      //Set the running flag
      buzzerRunning = true;
    }
  }

  void stop_buzzer() {
    // stop the timer
    *myTCCR1B &= 0b11111000;
    // set the flag to not running
    buzzerRunning = false;
    // set pin 13 LOW
    *port_b &= 0x7F;
  }
//-------------------

//-------------------
void setup() {
  //Set pin 11 as the trigger pin for the sensor. (output)
  // pinMode(TRIGGER, OUTPUT);
  *ddr_b |= (1 << 5);

  //Set pin 12 as the echo pin for the sensor. (input)
  //Pin 12 is PB6
  //This should be the GPIO code, but it isn't working.
  //I've tried switching it to an input with pullup, but that still doesn't work.
  //I can still access the data with GPIO
  //Please have mercy.
  // *ddr_b &= !(1 << 6);
  // *port_b &= !(1 << 6);
  pinMode(ECHO, INPUT); //Set pin 12 as the echo pin for the sensor.
  
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

  //Initialize the ADC
  adc_init();

  // Set pin 13 to output for buzzer
  *ddr_b |= (1 << 7);
  // Set pin 13 LOW
  *port_b &= !(1 << 7);
  // Initialize the buzzer timer for normal mode, with the TOV interrupt enabled
  init_timer_one();

  //Set LED's as outputs
  *ddr_a |= (1 << 0);
  *ddr_a |= (1 << 2);
  *ddr_a |= (1 << 4);
  *ddr_a |= (1 << 6);

  rtc.begin();

  potentiometerVal = adc_read();
  potentiometerDisplayTime = millis();

  UART_sendString("\r\n");
}


void loop() {
  potentiometerVal = ( (potentiometerVal * 9) + adc_read() ) / 10; //Stablilizing
  //700 units of range after clamping
  expectedBreathTime = (potentiometerVal > 900) ? (900) : (potentiometerVal); //Clamping
  expectedBreathTime = (potentiometerVal < 200) ? (200) : (potentiometerVal); //Clamping
  expectedBreathTime = 1000 + 5.7 * (expectedBreathTime - 200); //Range: 1000 microseconds to 5640 microseconds
  
  //Reset Button Logic--------------------
  if (currentState != 0) {
    if(!(*pin_g & (1 << 5))) { // pressed LOW
      lastState = 255;
      currentState = 1;
      errorCounter = 0;
      logBreathCount = 0;
      breathCount = 0;
      errorCounter = 0;
      errorCounterAlt = 0;
      potentiometerVal = 0;
      potentiometerDisplayTime = millis();
      lastRefreshTime = millis();
      previousTime = millis();
      loggingTime = millis();
      rapidBreathingCounter = 0;
      echoLength = 0;
      prevEchoLength = 0;
      breathGap = rapidBreathingThreshold;

      *port_a &= !(1 << 0);
      *port_a &= !(1 << 4);
      *port_a &= !(1 << 6);
      *port_a |= (1 << 2);

      UART_printTimestamp();
      UART_sendString(" - RESET\r\n");

      while (!(*pin_g & (1 << 5))); //wait release
    }
  }
  //Off Button Logic-----------------------
  if ((currentState != 0) && (currentState != 3)) {
    if(!(*pin_e & (1 << 5))) { // pressed LOW
      currentState = 0;
      
      while (!(*pin_e & (1 << 5))); //debounce
    }
  }

  if (currentState != lastState) {//On state-change
    
    lastState = currentState;
  
    //These activate once: when the state changes
    if (currentState == 0) {
      stop_buzzer();

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("OFF");

      
      *port_a &= !(1 << 2);
      *port_a &= !(1 << 4);
      *port_a &= !(1 << 6);
      *port_a |= (1 << 0);

      UART_printTimestamp();
      UART_sendString(" - OFF\r\n");
    } else if (currentState == 1) {
      stop_buzzer();

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("IDLE");

      UART_printTimestamp();
      UART_sendString(" - IDLE\r\n");

      *port_a &= !(1 << 0);
      *port_a &= !(1 << 4);
      *port_a &= !(1 << 6);
      *port_a |= (1 << 2);

    } else if (currentState == 2) {
      start_buzzer();

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ACTIVE");

      *port_a &= !(1 << 0);
      *port_a &= !(1 << 2);
      *port_a &= !(1 << 6);
      *port_a |= (1 << 4);

      UART_printTimestamp();
      UART_sendString(" - ACTIVE\r\n");

    } else if (currentState == 3) {
      stop_buzzer();

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ERROR");

      *port_a &= !(1 << 0);
      *port_a &= !(1 << 2);
      *port_a &= !(1 << 4);
      *port_a |= (1 << 6);

      UART_printTimestamp();
      UART_sendString(" - ERROR\r\n");

    }
  }

  //These activate repeatedtly
  if (currentState != 0) {
    if (currentState == 1) {
      do {
      //Idle Behavior
      getSensorData();
      if (echoLength < 400 || echoLength > 5000) {
        errorCounter += 2;
      }
      if (errorCounter > 0) {
        errorCounter--;
      }
      if (errorCounter > 20) {
        currentState = 3;
        break;
      }
      if (abs(echoLength - prevEchoLength) < 10) {
        errorCounterAlt += 2;
      }
      if (errorCounterAlt > 0) {
        errorCounterAlt--;
      }
      if (errorCounterAlt > 200) {
        currentState = 3;
        break;
      }
      processBreathing();

      if (millis() - loggingTime > 60000) {
        loggingTime = millis();
        UART_printTimestamp();
        UART_sendString(" - Sensor Data: ");
        UART_printFloat(echoLength);
        UART_sendString("\r\n");
        UART_printTimestamp();
        UART_sendString(" - BPM: ");
        UART_printInt(breathCount - logBreathCount);
        UART_sendString("\r\n");
        logBreathCount = breathCount;
      }

      if (millis() - potentiometerDisplayTime > 20000) {
        if (millis() - lastRefreshTime > 500) {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("BPM:");
          lcd.setCursor(0, 1);
          lcd.print("ADC:");
          lcd.setCursor(4, 0);
          lcd.print(60000/expectedBreathTime);
          lcd.setCursor(4, 1);
          lcd.print(adc_read());
          if (millis() - potentiometerDisplayTime > 30000) {
            potentiometerDisplayTime = millis();
          }
          lastRefreshTime = millis();
        }
      } else {
        lcd.setCursor(0, 1);
        lcd.print("SENSOR:");

        if (millis() - lastRefreshTime > 500) {
          lcd.clear();
          
          lcd.setCursor(0, 0);
          lcd.print("IDLE");

          lcd.setCursor(7, 1);
          lcd.print(echoLength);
          lastRefreshTime = millis();
        }
      }

      } while (false);
      
    } else if (currentState == 2) {
      //Active Behavior
      do {
      getSensorData();
      if (echoLength < 400 || echoLength > 25000) { //If something is too close or too far from the sensor.
        errorCounter += 2;
      }
      if (errorCounter > 0) {
        errorCounter--;
      }
      if (errorCounter > 20) { //Enter ERROR state
        currentState = 3;
        break;
      }
      if (abs(echoLength - prevEchoLength) < 10) { //If the sensor data is constant. I.E. completely stationary object in front of the sensor.
        errorCounterAlt += 2;
      }
      if (errorCounterAlt > 0) {
        errorCounterAlt--;
      }
      if (errorCounterAlt > 200) { //Enter error state
        currentState = 3;
        break;
      }
      processBreathing();

      if (millis() - loggingTime > 60000) {
        loggingTime = millis();
        UART_printTimestamp();
        UART_sendString("\r\n");
        UART_sendString(" - Sensor Data: ");
        UART_printFloat(echoLength);
        UART_sendString("\r\n");
        UART_sendString(" - BPM: ");
        UART_printInt(breathCount - logBreathCount);
        UART_sendString("\r\n");
        logBreathCount = breathCount;
      }

      if (millis() - potentiometerDisplayTime > 20000) {
        if (millis() - lastRefreshTime > 500) {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("BPM:");
          lcd.setCursor(0, 1);
          lcd.print("ADC:");
          lcd.setCursor(4, 0);
          lcd.print(60000/expectedBreathTime);
          lcd.setCursor(4, 1);
          lcd.print(adc_read());
          if (millis() - potentiometerDisplayTime > 30000) {
            potentiometerDisplayTime = millis();
          }
          lastRefreshTime = millis();
        }
      } else {
        lcd.setCursor(0, 1);
        lcd.print("SENSOR:");

        if (millis() - lastRefreshTime > 500) {
          lcd.clear();

          lcd.setCursor(0, 0);
          lcd.print("ACTIVE");

          lcd.setCursor(7, 1);
          lcd.print(echoLength);
          lastRefreshTime = millis();
        }
      }

      } while (false);

    } else if (currentState == 3) {
      //Error Behavior
      getSensorData();

      lcd.setCursor(0, 1);
      lcd.print("SENSOR:");

      if (millis() - lastRefreshTime > 500) {
        lcd.clear();

        lcd.setCursor(0, 0);
        lcd.print("ERROR");

        lcd.setCursor(7, 1);
        lcd.print(echoLength);
        lastRefreshTime = millis();
      }

    }
  }
}
//-------------------
