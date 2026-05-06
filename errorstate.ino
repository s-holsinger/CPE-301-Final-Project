//Done by Cydney
//Portion of the state: to be added in full code
//Define states: could be added alr?  

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
