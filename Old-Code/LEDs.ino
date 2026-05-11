//Done by Cydney
//Untested-
//Inside setup()
DDRB |= (1 << PB0); //pin 8
DDRB |= (1 << PB3); //pin 11
DDRB |= (1 << PB4); //pin 12
DDRB |= (1 << PB5); //pin 13

//LED update functions
void updateLEDs(unsigned char state) {

  // turn all OFF
  PORTB &= ~((1 << PB0) | (1 << PB3) | (1 << PB4) | (1 << PB5));

  if (state == OFF_STATE) {
    PORTB |= (1 << PB0);   // OFF LED
  }
  else if (state == IDLE_STATE) {
    PORTB |= (1 << PB3);   // IDLE LED
  }
  else if (state == ACTIVE_STATE) {
    PORTB |= (1 << PB4);   // ACTIVE LED
  }
  else if (state == ERROR_STATE) {
    PORTB |= (1 << PB5);   // ERROR LED
  }
}

//To be added in main code: 
//In very end of loop:-----
//updateLEDs(currentState);
//-------

