//CHEERS
#define RESET 1
#define SPINNER 10
#define CHANGESTATE 20
#define LOSE 31
#define WIN 32
#define ID_CHECK 40
#define COLOR_CHECK 50
#define VERIFY_ID_TIMER 100
#define COOLDOWN_TIMER 1000
#define ROUND_TIMER 1000
#define FLASH_TIMER_MIN 200
#define FLASH_TIMER_MID 400
#define FLASH_TIMER_MAX 600

//Basic functionality
Color colors[] = {RED, BLUE, GREEN, ORANGE, YELLOW, MAGENTA, WHITE, CYAN};
bool isPlayer;
bool isSpinner;
bool isSpinnerRing;
Timer stateChangeTimer;
byte faceCount;
bool buttonDoubleclicked;

//Flashing stuff
bool isFlasher;
bool flashOff;
int flashAmt;
Timer flashTimer;

//Players
bool searchingForMate;
byte bumpCount;
byte lastBumped;
bool allowBump;
int myColor[3];
byte myIndex;
bool checkColor;
Timer coolDownTimer;

//Spinner
Timer roundTimer;
bool roundTimerSet;
float pctDone;
byte firstIn;
byte lastIn;
byte checkedIn[6];
byte currentID;

enum GameState {
  CHEERS_SETUP,
  CHEERS
};

byte state;

//Initialize Vars
void init(){
  state = CHEERS_SETUP;
  isPlayer = false;
  isSpinner = false;
  isSpinnerRing = false;
  isFlasher = false;
  flashOff = true;
  flashAmt = FLASH_TIMER_MIN;
  searchingForMate = false;
  bumpCount = 0;
  lastBumped = 0;
  allowBump = false;
  myColor[0] = 255;
  myColor[1] = 0;
  myColor[1] = 0;
  myIndex = 0;
  checkColor = false;
  roundTimerSet = false;
  buttonDoubleclicked = false;
  for(byte i = 0; i < 6; i++)
    checkedIn[i] = 6;
  setColor(WHITE);
}

void setup(){
  init();
  flashTimer.set(FLASH_TIMER_MAX);
  randomize();
}

void loop() {
  //Flash Flashing blinks
  flashFlashers();
  // put your main code here, to run repeatedly:
   switch(state){
    case CHEERS_SETUP:
      cheersSetupLoop();
      break;
    case CHEERS:
      cheersGameLoop();
      break;
    default:
      break;
  }
  buttonDoubleclicked = buttonDoubleClicked();
}

void cheersSetupLoop() {
  isFlasher = false;
  faceCount = 0;
  
  FOREACH_FACE(f){
    if(!isValueReceivedOnFaceExpired(f)){
      faceCount++;
    }
  }
  
  //Check Players and Spinner
  isPlayer = faceCount <= 1;
  isSpinner = faceCount == 6;
  
  //Check for state Change
  FOREACH_FACE(f){
    if(!isValueReceivedOnFaceExpired(f)){
      if(getLastValueReceivedOnFace(f) >= CHANGESTATE){
        state = CHEERS;
        if(!isPlayer)
          setValueSentOnFace(getLastValueReceivedOnFace(f), (f+3)%6);
        if(isPlayer)
          myIndex = getLastValueReceivedOnFace(f) - CHANGESTATE;
      }
      else if(getLastValueReceivedOnFace(f) >= SPINNER){
        state = CHEERS;
        isSpinnerRing = true;
        setValueSentOnFace((getLastValueReceivedOnFace(f) - SPINNER) + CHANGESTATE, (f+3)%6);
      }
    }
   }
  
  //Change State Input
  if(buttonDoubleclicked && isSpinner){
    state = CHEERS;
    FOREACH_FACE(f){
      setValueSentOnFace(SPINNER+f,f);
    }
    buttonDoubleclicked = false;
  }

  //Draw
  if(isPlayer)
    setColor(GREEN);
  else if(isSpinner)
    setColor(CYAN);
  else
    setColor(WHITE);
}

void cheersGameLoop(){
  //PLAYER
  if(isPlayer){ 
    setValueSentOnAllFaces(ID_CHECK + myIndex);
    
    if(isFlasher){ //I'm blinking because I successfully bumped, so I need a new color
       if(coolDownTimer.isExpired()){
          isFlasher = false;
       }
    }     
    else if(isAlone()){ //Check number of faces to see if we're detached from the ring
      if(!searchingForMate){
        searchingForMate = true;
        checkColor = false;
        allowBump = true;
      }
    }
    else{
      searchingForMate = false;
    }
    
    if(!searchingForMate && allowBump){ //ID check
      allowBump = false;
    
      FOREACH_FACE(f){
        if(!isValueReceivedOnFaceExpired(f)){
          if(getLastValueReceivedOnFace(f) != lastBumped && getLastValueReceivedOnFace(f) >= ID_CHECK){
            bumpCount++;
            isFlasher = true;
            coolDownTimer.set(COOLDOWN_TIMER);
            lastBumped = getLastValueReceivedOnFace(f);
            bumpUpColor();
          }
        }
      }
    }

    //Loss check
    FOREACH_FACE(f){
      if(!isValueReceivedOnFaceExpired(f)){
        if(getLastValueReceivedOnFace(f) == LOSE){
          myColor[0] = 255;
          myColor[1] = 0;
        }
      }
    }
  }
  
  //SPINNER AND RING
  if(isSpinner || isSpinnerRing){
    //Timer
    if(!roundTimerSet){
      roundTimer.set(ROUND_TIMER);
      roundTimerSet = true;
    }
    else{
      if(roundTimer.isExpired()){
          isFlasher = false;
          setColor(RED);
          if(isSpinner && buttonDoubleclicked){
            setValueSentOnAllFaces(RESET);
            init();
          }
          FOREACH_FACE(f){
            if(!isValueReceivedOnFaceExpired(f)){
              if(getLastValueReceivedOnFace(f) >= ID_CHECK){
                currentID = getLastValueReceivedOnFace(f) - ID_CHECK;
                bool slotFound = false;
                for(byte i = 0; i < 6; i++){
                  if(!slotFound){
                    if(currentID == checkedIn[i])
                      slotFound = true; //Duplicate protection
                    else if(checkedIn[i] == 6){ //Default?
                      checkedIn[i] = currentID;
                      slotFound = true;
                    }
                  }
                }
              }
            }
          }
          
          firstIn = checkedIn[0];
          lastIn = checkedIn[5];

          if(lastIn != 6){
            //Show results
            setValueSentOnFace(LOSE,lastIn);
          }
        }
        else{
          isFlasher = true;
          pctDone = (float)(ROUND_TIMER - roundTimer.getRemaining()) / (float)ROUND_TIMER;
          pctDone *= 100.0;
          if(pctDone >= 75){
            setColor(RED);
            flashAmt = FLASH_TIMER_MIN;
          }
          else if(pctDone >= 50){
            setColor(YELLOW);
            flashAmt = FLASH_TIMER_MID;
          }
          else{
            setColor(GREEN);
            flashAmt = FLASH_TIMER_MAX;
          }
        }
    }
  }
  
  //RING
  if(!isPlayer && !isSpinner){
    FOREACH_FACE(f){
      if(!isValueReceivedOnFaceExpired(f)){
        if(getLastValueReceivedOnFace(f) >= ID_CHECK){
          setValueSentOnFace(ID_CHECK+f,(f+3)%6);
        }
        if(getLastValueReceivedOnFace(f) == LOSE){
          setValueSentOnFace(LOSE,(f+3)%6);
        }
      }
    }
  }
  
  if(isFlasher && flashOff){
    setColor(OFF);
  }
  else if(isPlayer){
    setColor(makeColorRGB(myColor[0], myColor[1], myColor[2]));
  }

  //RESET CHECK
  FOREACH_FACE(f){
    if(!isValueReceivedOnFaceExpired(f)){
      if(getLastValueReceivedOnFace(f) == RESET){
        setValueSentOnAllFaces(RESET);
        init();
      }
    }
  }
}

void bumpUpColor(){
  if(myColor[1] < 255){
    myColor[1] += 50;
    if(myColor[1] > 255)
      myColor[1] = 255;
  }
  else{
    myColor[0] -= 50;
  }
}

//Makes blinks flash from their color to black
void flashFlashers(){
  if(isFlasher && flashTimer.isExpired()){
    flashTimer.set(flashAmt);
    if(state == CHEERS){
      flashOff = !flashOff;
    }
  }
}
