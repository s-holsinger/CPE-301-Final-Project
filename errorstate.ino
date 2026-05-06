//Done by Cydney: needs to be fixed!!!!
//Portion of the state: to be added in full code
//Define states: Temporary example for placement 
#define OFF_STATE 0
#define IDLE_STATE 1
#define ACTIVE_STATE 2
#define ERROR_STATE 3

//State untested, will be later. 

//Error handler function:
void handleError(unsigned char &state, bool errorActive) {
//if system reports error -> go to ERROR state 
  if (errorActive && state != ERROR_STATE) {
    state = ERROR_STATE;
  } 
} 

void resetSystemData(bool &errorActive, int &breathCount, float &expectedBreathTime, int &rapidBreathingCounter){

  errorActive = false;
  breathCount = 0;
  expectedBreathTime = 1200;
  rapidBreathingCounter = 0;
  
}
//To be added into main code: 

//Global variables: 
//Above setup: -----
//volatile unsigned char currentstate = OFF_STATE 
//volatile Bool errorActive = false; 
//------
//Called in loop(): -----
//handleError(currentState, errorActive); 
//-------
//Inside reset button logic: -----
//resetSystemData(errorActive, breathCount, expectedBreathTime, rapidBreathingCounter);
//currentState = IDLE_STATE;
//--------
//In loop: (Error state output)
//if (currentState == ERROR_STATE) {
  //Serial.println("STATE: ERROR");
//}
