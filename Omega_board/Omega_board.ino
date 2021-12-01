Color colors[] = {RED, BLUE, GREEN, ORANGE, YELLOW, MAGENTA, WHITE};
bool isPlayer = false;
bool isSpinner = false;
bool isSpinnerRing = false;
bool isStrut = false;
bool isGameRing = false;
bool wasPlayer = false;
byte colorIndex = 0;
byte tempVal = 0;
bool ringStarted = false;

//Blinking stuff
bool isBlinker = false;
bool blinkOff = false;
Timer blinkTimer;
Timer stateChangeTimer;

//Spinner Stuff
bool isSpinning = false;
byte spinCurrent = 0;
Timer spinTimer;
int currentTimeAmount;
bool spinComplete = false;

enum GameState {
  SETUP,
  INGAME
};

byte state = SETUP;

#define SPINNERRING 20
#define PLAYER 21
#define STRUT_ONE 22
#define STRUT_TWO 23
#define GAMERING 30
#define UNASSIGNED 25
#define CHANGESTATE 26
#define BLINK_TIMER_MAX 200
#define STATE_TIMER 1000
#define SPIN_TIMER_MAX 1000
#define SPIN_TIMER_START 100
#define SPINTARGET 27

void setup() {
  // put your setup code here, to run once:
  blinkTimer.set(BLINK_TIMER_MAX);
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
  //Determine who is a racer and check for state change
   byte faceCount = 0;
   FOREACH_FACE(f){
    if(!isValueReceivedOnFaceExpired(f)){
      faceCount++;
      }
    }

  //Check Players and Spinner
  wasPlayer = isPlayer;
  isPlayer = faceCount <= 1;
  isSpinner = faceCount == 6;

  if(isPlayer){
    isGameRing = false;
    setValueSentOnAllFaces(PLAYER);
    if(wasPlayer)
      colorIndex %= 6;
    FOREACH_FACE(f){
      if(!isValueReceivedOnFaceExpired(f)){
        tempVal = getLastValueReceivedOnFace(f);
        if(tempVal == CHANGESTATE)
          state = INGAME;
      }
    }
  }
  else if(!isGameRing){
    colorIndex = 6;
    setValueSentOnAllFaces(UNASSIGNED);
  }

  //Create Spinner Ring
  isSpinnerRing = false;
  if(isSpinner){
    for(byte i = 0; i < 3; i++){
      setValueSentOnFace(i+10, i);
      setValueSentOnFace(i+10, i+3);
    }
    FOREACH_FACE(f){
      if(!isValueReceivedOnFaceExpired(f)){
        tempVal = getLastValueReceivedOnFace(f);
        if(tempVal == CHANGESTATE)
          state = INGAME;
        }
    }
  }
  
  if(!isSpinner && !isPlayer){
    FOREACH_FACE(f){
      if(!isValueReceivedOnFaceExpired(f)){
        tempVal = getLastValueReceivedOnFace(f);
        if(tempVal >= 10 && tempVal <=12){
          isSpinnerRing = true;
          colorIndex = tempVal - 10;
        }
        else if(tempVal == CHANGESTATE){
          state = INGAME;
        }
      }
    }
    if(isSpinnerRing){
      setValueSentOnAllFaces(SPINNERRING);
      isGameRing = false;
    }
  }

  //Create Struts
  isStrut = false;
  if(!isSpinner && !isPlayer && !isSpinnerRing){
    FOREACH_FACE(f){
      if(!isValueReceivedOnFaceExpired(f)){
        tempVal = getLastValueReceivedOnFace(f);
        if(tempVal == SPINNERRING){ //Neighbor is ring, i am a strut
          isStrut = true;
           setValueSentOnAllFaces(STRUT_ONE);
        }
        else if(tempVal == STRUT_ONE){
          isStrut = true;
          isGameRing = false;
          setValueSentOnAllFaces(STRUT_TWO);
        }
        else if(tempVal == CHANGESTATE)
          state = INGAME;
        }
    }

    if(isStrut){
      //colorIndex = 4; 
      isGameRing = false;
    }
  }

  //Create Game Ring
  if(!isSpinner && !isPlayer && !isSpinnerRing && !isStrut){
    isGameRing = true;
    isBlinker = false;
    FOREACH_FACE(f){
      if(!isValueReceivedOnFaceExpired(f)){
        tempVal = getLastValueReceivedOnFace(f);
        //Make "blinkers"
        if(tempVal == STRUT_TWO && !isValueReceivedOnFaceExpired((f+1)%6) && !isValueReceivedOnFaceExpired((f-1)%6))
          isBlinker = true;
        if(tempVal >= GAMERING && !ringStarted){
          colorIndex = (tempVal - GAMERING + 1) % 3;
          ringStarted = true;
          state = INGAME;
          if(!isValueReceivedOnFaceExpired((f+3)%6))
            setValueSentOnFace(GAMERING + colorIndex, (f+3)%6);
          else if(!isValueReceivedOnFaceExpired((f+4)%6))
            setValueSentOnFace(GAMERING + colorIndex, (f+4)%6);
          else if(!isValueReceivedOnFaceExpired((f+2)%6))
            setValueSentOnFace(GAMERING + colorIndex, (f+2)%6);
        }
      }
    }
  }
  
  //Check for player input (color change)
  if(buttonPressed() && isPlayer){
    colorIndex++;
    colorIndex %= COUNT_OF(colors)-1;
  }

  //Populate game ring
  if(buttonDoubleClicked() && isGameRing && !ringStarted){
    colorIndex = 0;
    FOREACH_FACE(f){
      if(!isValueReceivedOnFaceExpired(f)){
        tempVal = getLastValueReceivedOnFace(f);
        if(tempVal != STRUT_TWO && !ringStarted){
          setValueSentOnFace(GAMERING, f);
          ringStarted = true;
          state = INGAME;
        }
      }
    }
  }

  //Draw
  blinkBlinkers();
  
  //Set Color
  if(isSpinner || (isBlinker && blinkOff))
    setColor(OFF);
  else
    setColor(colors[colorIndex]);  

  if(state == INGAME)
    stateChangeTimer.set(STATE_TIMER);
}

void inGameLoop(){
  if(stateChangeTimer.isExpired())
    setValueSentOnAllFaces(CHANGESTATE);

  if(isSpinner && buttonDoubleClicked() && !isSpinning){
    spinSpinner();
  }
  
  //Draw
  blinkBlinkers();
  
  //Set Color
  if(isSpinner || (isBlinker && blinkOff)){
    setColor(OFF);
    if(spinComplete)
      setColor(colors[spinCurrent % 3]);
  }
  else
    setColor(colors[colorIndex]);

  if(isSpinning && (isSpinner || isSpinnerRing ))
    spinSpinner();
}

void blinkBlinkers(){
  if(isBlinker && blinkTimer.isExpired()){
    blinkTimer.set(BLINK_TIMER_MAX);
    if(ringStarted){
      blinkOff = !blinkOff;
    }
  }
}

void spinSpinner(){
  if(!isSpinning){
    isSpinning = true;
    spinCurrent = random(5);
    spinTimer.set(SPIN_TIMER_START);
    currentTimeAmount = SPIN_TIMER_START;
    if(isSpinner){
      setValueOnAllFaces(
      setValueSentOnFace(SPINTARGET, spinCurrent);
      spinComplete = false;
    }
  }
  else{
    if(spinTimer.isExpired()){
      if(isSpinner)
         setValueSentOnFace(UNASSIGNED, spinCurrent);
      if(currentTimeAmount < SPIN_TIMER_MAX){
        currentTimeAmount *= 1.1;
        spinTimer.set(currentTimeAmount);
        spinCurrent = (spinCurrent + 1) % 6;
        setValueSentOnFace(SPINTARGET, spinCurrent);
      }
      else{
        isSpinning = false;
        spinComplete = true;
      }
    }
  }

  if(isSpinner){
    setColorOnFace(colors[spinCurrent % 3], spinCurrent);
  }
  else{
  
    FOREACH_FACE(f){
      if(getLastValueReceivedOnFace(f) == SPINTARGET){
        setColorOnFace(WHITE, f);
      }
      else{
        setColorOnFace(colors[colorIndex], f);
      }
    }
  }
}
