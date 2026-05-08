//Buzzer code
//Done by Cydney
//Add into main code, and needs to be tested
//Defines incase pin needs to be changed

#define BUZZER_PIN PD5
//In setup()

DDRD |= (1<< BUZZER_PIN);


//Buzzerr function

void updateBuzzer(unsigned char state){

  if (state == ERROR_STATE){
    PORTD |= (1 << BUZZER_PIN);
  }
  else {
    PORTD &= ~(1 << BUZZER_PIN);
  }
}
//in loop()
//updateBuzzer(currentState);
  
