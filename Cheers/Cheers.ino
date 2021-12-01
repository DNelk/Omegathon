/*
 * SETUP: Create a 7-blink cluster (6 blinks in a ring, with one at the center) place one blink on each outer blink so that they only have one connection. (They will turn green)
 * When all players are ready, double click the center (cyan) blink.
 * 
 * GAMEPLAY: All players  bump their blinks together until they flash. They repeat this until the timer runs out. Players cannot bump with the same person twice in a row.
 * When the timer runs out, the players with the least bumps (most red colored blink), and the player who connects to the ring last lose.
 */
 
#define CHANGESTATE 10
#define NEEDSCOLOR 21
#define IDLE_STATE 22
#define LOSE 31
#define WIN 32
#define ID_CHECK 40
#define COLOR_CHECK 50
#define STATE_TIMER 500
#define BLINK_TIMER_MIN 200
#define BLINK_TIMER_MID 400
#define BLINK_TIMER_MAX 600
#define VERIFY_ID_TIMER 100
#define COOLDOWN_TIMER 1000
#define ROUND_TIMER 10000


Color colors[] = {RED, BLUE, GREEN, ORANGE, YELLOW, MAGENTA, WHITE, CYAN};
bool isCup = false;
bool isBrain = false;
Timer stateChangeTimer;
bool stateChangeComplete = false;

//Game logic
byte faceCount;

//TODO add game end timer

//Cups
bool colorAssigned = false;
bool searchingForMate = false;
byte bumpCount = 0;
byte lastBumped = 0;
bool allowBump = false;
int myColor[] = {255,0,0};
byte myIndex = 0;
bool checkColor = false;
Timer coolDownTimer;

//Brain
Timer roundTimer;
bool roundTimerSet = false;
float pctDone;
byte firstIn;
byte lastIn;
byte checkedIn[6];
byte currentID;

//Ring
bool waitingForBrain = true;

//Blinking stuff
bool isBlinker = false;
bool blinkOff = false;
Timer blinkTimer;
int blinkAmt = BLINK_TIMER_MIN;

enum GameState{
  SETUP,
  INGAME
};
byte state = SETUP;

void setup() {
  // put your setup code here, to run once:
  randomize();
}

void loop() {
  // put your main code here, to run repeatedly:
  switch(state){
    case SETUP:
      setupLoop();
      break;
    case INGAME:
      inGameLoop();
      break;
    default:
      break;
  }
}

void setupLoop() {
  //Determine who is a player and check for state change
   faceCount = 0;
   FOREACH_FACE(f){
    if(!isValueReceivedOnFaceExpired(f)){
      faceCount++;
      if(getLastValueReceivedOnFace(f) >= CHANGESTATE){
        state = INGAME;
        if(!isCup)
          setValueSentOnFace(getLastValueReceivedOnFace(f), (f+3)%6);
        if(isCup)
          myIndex = getLastValueReceivedOnFace(f) - CHANGESTATE;
      }
    }
   }

  //Check Players and Spinner
  isCup = faceCount <= 1;
  isBrain = faceCount == 6;

  //Change State
  if(buttonDoubleClicked() && isBrain){
    state = INGAME;
    FOREACH_FACE(f){
      setValueSentOnFace(CHANGESTATE+f,f);
    }
  }

  //Draw
  if(isCup)
    setColor(GREEN);
  else if(isBrain)
    setColor(CYAN);
  else
    setColor(WHITE);
}

void inGameLoop(){
  //CUP
  if(isCup){ 
    setValueSentOnAllFaces(ID_CHECK + myIndex);
    
    if(isBlinker){ //I'm blinking because I successfully bumped, so I need a new color
       if(coolDownTimer.isExpired()){
          isBlinker = false;
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
            isBlinker = true;
            coolDownTimer.set(COOLDOWN_TIMER);
            lastBumped = getLastValueReceivedOnFace(f);
            bumpUpColor();
          }
          if(getLastValueReceivedOnFace(f) == LOSE){
            myColor[0] = 255;
            myColor[1] = 0;
          }
        }
      }
    }

  }
  //BRAIN
  if(isBrain){
    //Timer
    if(!roundTimerSet){
      roundTimer.set(ROUND_TIMER);
      roundTimerSet = true;
    }
    else{
      if(roundTimer.isExpired()){
          isBlinker = false;
          /*FOREACH_FACE(f){
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
          }*/
        }
        else{
          isBlinker = true;
          pctDone = (float)(ROUND_TIMER - roundTimer.getRemaining()) / (float)ROUND_TIMER;
          pctDone *= 100.0;
          if(pctDone >= 75){
            setColor(RED);
            blinkAmt = BLINK_TIMER_MIN;
          }
          else if(pctDone >= 50){
            setColor(YELLOW);
            blinkAmt = BLINK_TIMER_MID;
          }
          else{
            setColor(GREEN);
            blinkAmt = BLINK_TIMER_MAX;
          }
        }
    }
  }
  
  //RING
  if(!isCup && !isBrain){
    FOREACH_FACE(f){
      if(!isValueReceivedOnFaceExpired(f)){
        if(getLastValueReceivedOnFace(f) >= ID_CHECK){
          setValueSentOnFace(ID_CHECK+f,(f+3)%6);
        }
      }
    }
  }
  //Set Color
  blinkBlinkers();
  
  if(isBlinker && blinkOff){
    setColor(OFF);
  }
  else if(isCup){
    setColor(makeColorRGB(myColor[0], myColor[1], myColor[2]));
  }
  
}

void blinkBlinkers(){
  if(isBlinker && blinkTimer.isExpired()){
    blinkTimer.set(blinkAmt);
    if(state == INGAME){
      blinkOff = !blinkOff;
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
