// STATE MACHINE BACKBONE - will be expanded:)
//Serial library used for debugging, will be deleted

//STATE (will be expanded later)
volatile unsigned char currentState = 0; //0 = OFF, 1 = IDLE (for now)
volatile unsigned char startPressed = 0; //FLAG

unsigned char lastState = 255;

//ISR-------------
void startISR() {
  Serial.println("innterupt fired");
   startPressed = 1;
 }


void setup() {

  Serial.begin(9600);
//Start Button ---------------------------
  DDRD &= ~(1 << PD2);  //PIN 2 input
  PORTD |= (1 << PD2); //Enable pull up resistor on PD2

  attachInterrupt(digitalPinToInterrupt(2), startISR, FALLING);  //Interrupt on falling edge (button press)
//----------------------------------------
//Off Button -----------------------------
  DDRD &= ~(1 << PD3);  //PIN 3 input
  PORTD |= (1 << PD3);  //pull up
//----------------------------------------
//Reset Button ---------------------------
  DDRD &= ~(1 << PD4);  //PIN 4 input
  PORTD |= (1 << PD4);  //pull up
//----------------------------------------
  currentState = 0;
  lastState = 255;
  
  Serial.println("System started in OFF state"); //TO TEST***
}

void loop() {

  //Reset Button Logic--------------------
    if(!(PIND & (1<< PD4))) { // pressed LOW
      currentState = 1;
      startPressed = 0;

      Serial.println("RESET -> IDLE (DATA CLEARED)");

      while (!(PIND & (1<< PD4))); //wait release
    }

  //Off Button Logic-----------------------
  if (currentState != 0) {
    if(!(PIND & (1<< PD3))) { // pressed LOW
      currentState = 0;
      startPressed = 0;
      
      while (!(PIND & (1<< PD3))); //debounce
    }
  }

  //Start button- OFF STATE LOGIC (FOR NOW)-
  if (currentState == 0) {
    
    if (startPressed == 1) {
      startPressed = 0;
      currentState = 1; //moves to IDLE
    } //----------------------
  }
  if (currentState != lastState) {//WHOLE IF STATEMENT TEMP FOR TEST***
    
      lastState = currentState;
  
    if (currentState == 0) {
      Serial.println("STATE: OFF");
    }
    else if (currentState == 1) {
      Serial.println("STATE: IDLE");
    }  
  } 
}
