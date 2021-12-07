#define CHANGESTATE_BOARD 1
#define CHANGESTATE_RLGL 2
#define CHANGESTATE_CHEERS 3
#define CHANGESTATE_CHAIRS 4
#define SPINNERRING 20
#define PLAYER 21
#define STRUT_ONE 22
#define STRUT_TWO 23
#define UNASSIGNED 25
#define SPINTARGET 27
#define GAMERING 30
#define FLASH_TIMER_MIN 200
#define FLASH_TIMER_MID 400
#define FLASH_TIMER_MAX 600
#define STATE_TIMER_LONG 1000
#define STATE_TIMER_SHORT 500
#define SPIN_TIMER_MAX 200
#define SPIN_TIMER_START 100
#define CHANGE_GAME_TIMER 2000

//Basic Functionality
Color colors[] = {RED, BLUE, GREEN, ORANGE, YELLOW, MAGENTA, WHITE, CYAN, OFF};
bool isPlayer = false;
bool isSpinner = false;
bool isSpinnerRing = false;
bool isStrut = false;
bool isGameRing = false;
bool wasPlayer = false;
byte colorIndex = 0;
byte tempVal = 0;
bool ringStarted = false;
Timer stateChangeTimer;

//Flashing stuff
bool isFlasher = false;
bool flashOff = true;
int flashAmt = FLASH_TIMER_MIN;
Timer flashTimer;

//Spinner Stuff
bool isSpinning = false;
byte spinCurrent = 0;
Timer spinTimer;
int currentTimeAmount;
bool spinComplete = false;
Timer displayChangeTimer;

enum GameState {
  SETUP,
  BOARD,
  RLGL_SETUP,
  RLGL,
  CHEERS_SETUP,
  CHEERS,
  CHAIRS_SETUP,
  CHAIRS
};

byte state = SETUP;

void setup() {
  // put your setup code here, to run once:
  flashTimer.set(FLASH_TIMER_MAX);
  randomize();
}

void loop() {
  //Flash Flashing blinks
  flashFlashers();
  // put your main code here, to run repeatedly:
   switch(state){
    case SETUP:
      setupLoop();
      break;
    case BOARD:
      boardLoop();
      break;
    case RLGL_SETUP:
      //rlglSetupLoop();
      break;
    case RLGL:
      //rlglGameLoop();
      break;
    case CHEERS_SETUP:
      //cheersSetupLoop();
      break;
    case CHEERS:
      //cheersGameLoop();
      break;
    case CHAIRS_SETUP:
      //chairsSetupLoop();
      break;
    case CHAIRS:
      //chairsGameLoop();
      break;
    default:
      break;
  }

  buttonDoubleClicked();
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
        if(tempVal == CHANGESTATE_BOARD)
          state = BOARD;
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
        if(tempVal == CHANGESTATE_BOARD)
          state = BOARD;
        }
    }
    colorIndex = 7;
  }
  
  if(!isSpinner && !isPlayer){
    FOREACH_FACE(f){
      if(!isValueReceivedOnFaceExpired(f)){
        tempVal = getLastValueReceivedOnFace(f);
        if(tempVal >= 10 && tempVal <=12){
          isSpinnerRing = true;
          colorIndex = tempVal - 10;
          setValueSentOnFace(SPINNERRING, (f+3)%6);
        }
        else if(tempVal == CHANGESTATE_BOARD){
          state = BOARD;
        }
      }
    }
    if(isSpinnerRing){    
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
           setValueSentOnFace(STRUT_ONE, (f+3)%6);
           colorIndex = 6;
        }
        else if(tempVal == STRUT_ONE){
          isStrut = true;
          isGameRing = false;
          setValueSentOnFace(STRUT_TWO, (f+3)%6);
          colorIndex = 6;
        }
        else if(tempVal == CHANGESTATE_BOARD )
          state = BOARD;
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
    isFlasher = false;
    FOREACH_FACE(f){
      if(!isValueReceivedOnFaceExpired(f)){
        tempVal = getLastValueReceivedOnFace(f);
        //Make "flashers"
        if(tempVal == STRUT_TWO)
          isFlasher = true;
        if(tempVal >= GAMERING && !ringStarted){
          colorIndex = (tempVal - GAMERING + 1) % 3;
          ringStarted = true;
          state = BOARD;
          if(!isValueReceivedOnFaceExpired((f+3)%6))
            setValueSentOnFace(GAMERING + colorIndex, (f+3)%6);
          else if(!isValueReceivedOnFaceExpired((f+4)%6) && getLastValueReceivedOnFace((f+4)%6) == UNASSIGNED)
            setValueSentOnFace(GAMERING + colorIndex, (f+4)%6);
          else if(!isValueReceivedOnFaceExpired((f+2)%6) && getLastValueReceivedOnFace((f+2)%6) == UNASSIGNED)
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
          state = BOARD;
        }
      }
    }
  }

  //Set Color
  if(isFlasher && flashOff)
    setColor(OFF);
  else
    setColor(colors[colorIndex]);  

  if(state == BOARD)
    stateChangeTimer.set(STATE_TIMER_LONG);
}

void boardLoop(){
  if(stateChangeTimer.isExpired()){
    setValueSentOnAllFaces(CHANGESTATE_BOARD);
  }
  
  if(isSpinner && buttonDoubleClicked() && !isSpinning){
    spinSpinner();
  }

  //Check for new State changes
  FOREACH_FACE(f){
    if(!isValueReceivedOnFaceExpired(f)){
      tempVal = getLastValueReceivedOnFace(f);
      if(tempVal == CHANGESTATE_RLGL)
        state = RLGL_SETUP;
      if(tempVal == CHANGESTATE_CHEERS)
        state = CHEERS_SETUP;
      if(tempVal == CHANGESTATE_CHAIRS)
        state = CHAIRS_SETUP;
    }
  }
  
  //Draw
  if(isFlasher && flashOff){
    setColor(OFF);
  }
  else{
    if(spinComplete && isSpinner){
      setColor(colors[spinCurrent % 3]);
      if(displayChangeTimer.isExpired()){
        switch((spinCurrent % 3)){
          case 0:
            state = CHEERS_SETUP;
            break;
          case 1:
            state = CHEERS_SETUP;
            break;
          case 2:
            state = CHEERS_SETUP;
            break;
        }
      }
    }
    else{
      setColor(colors[colorIndex]);
    }
  }

  if(isSpinning && isSpinner)
    spinSpinner();

  if(state == RLGL_SETUP)
    stateChangeTimer.set(STATE_TIMER_SHORT);
  if(state == CHEERS_SETUP)
    stateChangeTimer.set(STATE_TIMER_SHORT);
  if(state == CHAIRS_SETUP)
    stateChangeTimer.set(STATE_TIMER_SHORT);
}

//Makes blinks flash from their color to black
void flashFlashers(){
  if(isFlasher && flashTimer.isExpired()){
    flashTimer.set(flashAmt);
    if(state == BOARD || state == CHEERS){
      flashOff = !flashOff;
    }
  }
}

//Spins the spinner
void spinSpinner(){
  if(!isSpinning){
    isSpinning = true;
    colorIndex = 8;
    spinCurrent = random(5);
    spinTimer.set(SPIN_TIMER_START);
    currentTimeAmount = SPIN_TIMER_START;
    if(isSpinner){
      //setValueOnAllFaces(
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
        isFlasher = true;
        displayChangeTimer.set(CHANGE_GAME_TIMER);
      }
    }
    else{
      if(isSpinner){
        setColorOnFace(colors[spinCurrent % 3], spinCurrent);
      }
    }
  }
}

////RED LIGHT GREEN LIGHT
//
//////CHEERS
////
//#define CHANGESTATE 10
//#define NEEDSCOLOR 21
//#define IDLE_STATE 22
//#define LOSE 31
//#define WIN 32
//#define ID_CHECK 40
//#define COLOR_CHECK 50
//#define VERIFY_ID_TIMER 100
//#define COOLDOWN_TIMER 1000
//#define ROUND_TIMER 10000
////
//////Players
//bool colorAssigned = false;
//bool searchingForMate = false;
//byte bumpCount = 0;
//byte lastBumped = 0;
//bool allowBump = false;
//int myColor[] = {255,0,0};
//byte myIndex = 0;
//bool checkColor = false;
//Timer coolDownTimer;
////
//////Spinner
//Timer roundTimer;
//bool roundTimerSet = false;
//float pctDone;
//byte firstIn;
//byte lastIn;
//byte checkedIn[6];
//byte currentID;
//
//void cheersSetupLoop() {
//  /*if(resetVars)
//    cheersReset();*/
//  isFlasher = false;
//  
//  if(stateChangeTimer.isExpired()){
//    setValueSentOnAllFaces(CHANGESTATE_CHEERS);
//  }
//  //Check for state Change
//  FOREACH_FACE(f){
//    if(!isValueReceivedOnFaceExpired(f)){
//      if(getLastValueReceivedOnFace(f) >= CHANGESTATE){
//        state = CHEERS;
//        if(!isPlayer)
//          setValueSentOnFace(getLastValueReceivedOnFace(f), (f+3)%6);
//        if(isPlayer)
//          myIndex = getLastValueReceivedOnFace(f) - CHANGESTATE;
//      }
//    }
//   }
//  
//  //Change State Input
//  if(buttonDoubleClicked() && isSpinner){
//    state = CHEERS;
//    FOREACH_FACE(f){
//      setValueSentOnFace(CHANGESTATE+f,f);
//    }
//  }
//
//  //Draw
//  if(isPlayer)
//    setColor(GREEN);
//  else if(isSpinner)
//    setColor(CYAN);
//  else
//    setColor(WHITE);
//}
//
//void cheersGameLoop(){
//  //PLAYER
//  if(isPlayer){ 
//    setValueSentOnAllFaces(ID_CHECK + myIndex);
//    
//    if(isFlasher){ //I'm blinking because I successfully bumped, so I need a new color
//       if(coolDownTimer.isExpired()){
//          isFlasher = false;
//       }
//    }     
//    else if(isAlone()){ //Check number of faces to see if we're detached from the ring
//      if(!searchingForMate){
//        searchingForMate = true;
//        checkColor = false;
//        allowBump = true;
//      }
//    }
//    else{
//      searchingForMate = false;
//    }
//    
//    if(!searchingForMate && allowBump){ //ID check
//      allowBump = false;
//    
//      FOREACH_FACE(f){
//        if(!isValueReceivedOnFaceExpired(f)){
//          if(getLastValueReceivedOnFace(f) != lastBumped && getLastValueReceivedOnFace(f) >= ID_CHECK){
//            bumpCount++;
//            isFlasher = true;
//            coolDownTimer.set(COOLDOWN_TIMER);
//            lastBumped = getLastValueReceivedOnFace(f);
//            bumpUpColor();
//          }
//        }
//      }
//    }
//
//    //Loss check
//    /*FOREACH_FACE(f){
//      if(!isValueReceivedOnFaceExpired(f)){
//        if(getLastValueReceivedOnFace(f) == LOSE){
//          myColor[0] = 255;
//          myColor[1] = 0;
//        }
//      }
//    }*/
//  }
//  //BRAIN
//  if(isSpinner){
//    //Timer
//    if(!roundTimerSet){
//      roundTimer.set(ROUND_TIMER);
//      roundTimerSet = true;
//    }
//    else{
//      if(roundTimer.isExpired()){
//          isFlasher = false;
//          /*FOREACH_FACE(f){
//            if(!isValueReceivedOnFaceExpired(f)){
//              if(getLastValueReceivedOnFace(f) >= ID_CHECK){
//                currentID = getLastValueReceivedOnFace(f) - ID_CHECK;
//                bool slotFound = false;
//                for(byte i = 0; i < 6; i++){
//                  if(!slotFound){
//                    if(currentID == checkedIn[i])
//                      slotFound = true; //Duplicate protection
//                    else if(checkedIn[i] == 6){ //Default?
//                      checkedIn[i] = currentID;
//                      slotFound = true;
//                    }
//                  }
//                }
//              }
//            }
//          }
//          
//          firstIn = checkedIn[0];
//          lastIn = checkedIn[5];
//
//          if(lastIn != 6){
//            //Show results
//            setValueSentOnFace(LOSE,lastIn);
//          }*/
//        }
//        else{
//          isFlasher = true;
//          pctDone = (float)(ROUND_TIMER - roundTimer.getRemaining()) / (float)ROUND_TIMER;
//          pctDone *= 100.0;
//          if(pctDone >= 75){
//            setColor(RED);
//            flashAmt = FLASH_TIMER_MIN;
//          }
//          else if(pctDone >= 50){
//            setColor(YELLOW);
//            flashAmt = FLASH_TIMER_MID;
//          }
//          else{
//            setColor(GREEN);
//            flashAmt = FLASH_TIMER_MAX;
//          }
//        }
//    }
//  }
//  
//  //RING
//  if(!isPlayer && !isSpinner){
//    FOREACH_FACE(f){
//      if(!isValueReceivedOnFaceExpired(f)){
//        if(getLastValueReceivedOnFace(f) >= ID_CHECK){
//          setValueSentOnFace(ID_CHECK+f,(f+3)%6);
//        }
//      }
//    }
//  }
//  
//  if(isFlasher && flashOff){
//    setColor(OFF);
//  }
//  else if(isPlayer){
//    setColor(makeColorRGB(myColor[0], myColor[1], myColor[2]));
//  }
//  
//}
//
//void bumpUpColor(){
//  if(myColor[1] < 255){
//    myColor[1] += 50;
//    if(myColor[1] > 255)
//      myColor[1] = 255;
//  }
//  else{
//    myColor[0] -= 50;
//  }
//}

////MUSICAL CHAIRS
//#define VERIFY_ID_TIMER 100
//#define ROUND_TIMER_EASY 30000
//#define ROUND_TIMER_MEDIUM 20000
//#define ROUND_TIMER_HARD 10000
//#define NEEDSCOLOR 21
//#define IDLE_STATE 22
//#define COLOR_RECEIVED 23
//#define SETTINGCOLORPLAYER 40
//#define SETTINGCOLORSPACES 50
//
////Other logic
//byte faceCount;
//
////Players
//byte myColor = 6;
//
////Spinner
//byte rnd;
//byte currentColor;
//byte foundCount;
//bool generateNewColor = true;
//bool assignColorsToPlayers = true;
//bool assignColorsToSpaces = false;
//byte color1 = 0;
//byte color2 = 0;
//byte color3 = 0;
//
////Ring
//bool hasColor = false;
//
//void chairsSetupLoop() {
//  if(stateChangeTimer.isExpired()){
//    setValueSentOnAllFaces(CHANGESTATE_CHAIRS);
//  }
//  //Determine who is a player and check for state change
//   faceCount = 0;
//   FOREACH_FACE(f){
//    if(!isValueReceivedOnFaceExpired(f)){
//      faceCount++;
//      if(getLastValueReceivedOnFace(f) >= CHANGESTATE){
//        state = CHAIRS;
//        if(!isPlayer)
//          setValueSentOnFace(getLastValueReceivedOnFace(f), (f+3)%6);
//        if(isPlayer)
//          myIndex = getLastValueReceivedOnFace(f) - CHANGESTATE;
//      }
//    }
//   }
//
//  //Change State
//  if(buttonDoubleClicked() && isSpinner){
//    state = CHAIRS;
//    FOREACH_FACE(f){
//      setValueSentOnFace(CHANGESTATE+f,f);
//    }
//  }
//
//    //Draw
//  if(isPlayer)
//    setColor(GREEN);
//  else if(isSpinner)
//    setColor(CYAN);
//  else
//    setColor(WHITE);
//}
//
//void chairsGameLoop(){
//  //Player
//  if(isPlayer){ 
//    faceCount = 0;
//    FOREACH_FACE(f){
//      if(!isValueReceivedOnFaceExpired(f)){
//        faceCount++; 
//        if(!colorAssigned){ //I need a color
//          if(getLastValueReceivedOnFace(f) >= SETTINGCOLORPLAYER && getLastValueReceivedOnFace(f) < SETTINGCOLORSPACES){
//           myColor = getLastValueReceivedOnFace(f) - SETTINGCOLORPLAYER; //Get my color from the ring
//           isFlasher = false;
//           colorAssigned = true;
//           setValueSentOnAllFaces(COLOR_RECEIVED);
//          }
//        }
//      }
//    }
//  }
//  //BRAIN
//  if(isSpinner){
//    myColor = 7;
//    if(assignColorsToPlayers){
//      byte assigned[] = {6,6,6,6,6,6};
//      foundCount = 0;
//      generateNewColor = true;
//      //Generate new colors
//      color1 = random(5);
//      color2 = color1;
//      while(color2 == color1){
//        color2 = random(5);
//      }
//      color3 = color2;
//      while(color3 == color2 || color3 == color1){
//        color3 = random(5);
//      }
//      //Distribute colors along paths
//      for(byte n = 0; n < 6; n++){
//        currentColor = 6;
//        generateNewColor = true;
//        while(generateNewColor){
//          foundCount = 0;
//          rnd = random(2);
//          switch(rnd){
//            case 0:
//              currentColor = color1;
//              break;
//            case 1:
//              currentColor = color2;
//              break;
//            case 2:
//              currentColor = color3;
//              break; 
//           }
//
//           //Check if this color has already been assigned twice
//           for(byte i = 0; i < 6; i++){
//            if(assigned[i] == currentColor)
//              foundCount++;
//           }
//            
//           if(foundCount >= 2)
//            generateNewColor = true;
//           else{
//            generateNewColor = false;
//            assigned[n] = currentColor;
//            setValueSentOnFace(SETTINGCOLORPLAYER+currentColor, n);
//           }
//          }
//        }
//        assignColorsToPlayers = false;
//      }
//
//      if(assignColorsToSpaces){
//         byte assignedSpaces[] = {6,6,6};
//         foundCount = 0;
//         generateNewColor = true;
//         //Pick our spots and colors
//         for(byte n = 0; n < 6; n++){
//          currentColor = 6;
//          generateNewColor = true;
//          while(generateNewColor){
//            foundCount = 0;
//            rnd = random(2);
//            switch(rnd){
//              case 0:
//                currentColor = color1;
//                break;
//              case 1:
//                currentColor = color2;
//                break;
//              case 2:
//                currentColor = color3;
//                break; 
//            }
//
//            //Check if this color has already been assigned twice
//            for(byte i = 0; i < 6; i++){
//              if(assignedSpaces[i] == currentColor)
//                foundCount++;
//            }
//            
//            if(foundCount >= 2)
//              generateNewColor = true;
//            else{
//              generateNewColor = false;
//              assignedSpaces[n] = currentColor;
//              assignedSpaces[n+1] = currentColor;
//              setValueSentOnFace(SETTINGCOLORSPACES+currentColor, n);
//              setValueSentOnFace(SETTINGCOLORSPACES+currentColor, n+1);
//              n++;
//            }
//          }
//        }
//        assignColorsToSpaces = false;
//      }
//      
//      FOREACH_FACE(f){
//        if(!isValueReceivedOnFaceExpired(f)){
//          if(getLastValueReceivedOnFace(f) == NEEDSCOLOR && stateChangeTimer.isExpired()){
//            assignColorsToSpaces = true;
//            stateChangeTimer.set(1000);
//          }
//        }
//      }
//  }
//  //RING
//  if(!isPlayer && !isSpinner){
//    FOREACH_FACE(f){
//      if(!isValueReceivedOnFaceExpired(f)){
//        if(getLastValueReceivedOnFace(f) >= SETTINGCOLORPLAYER && getLastValueReceivedOnFace(f) < SETTINGCOLORSPACES){
//          setValueSentOnFace(getLastValueReceivedOnFace(f),(f+3)%6);
//        }
//        if(getLastValueReceivedOnFace(f) >= SETTINGCOLORSPACES && !hasColor){
//          myColor = getLastValueReceivedOnFace(f) - SETTINGCOLORSPACES;
//          setValueSentOnAllFaces(IDLE_STATE);
//          hasColor = true;
//        }
//        if(getLastValueReceivedOnFace(f) == COLOR_RECEIVED && !hasColor){
//          setValueSentOnFace(NEEDSCOLOR, (f+3)%6);
//        } 
//      }
//    }
//  }
//
//
//  //Timer
//  if(!roundTimerSet){
//    roundTimer.set(ROUND_TIMER_HARD);
//    roundTimerSet = true;
//  }
//  else{
//    if(roundTimer.isExpired()){
//        if(isSpinner){
//          isFlasher = false;
//          }
//        if(isPlayer){
//          isFlasher = true;
//          myColor = 0;
//          FOREACH_FACE(f){
//            if(!isValueReceivedOnFaceExpired(f)){
//              if(getLastValueReceivedOnFace(f) == IDLE_STATE){
//                myColor = 2;
//              }
//            }
//          }
//        }
//      }
//      else{
//        if(isSpinner){
//          isFlasher = true;
//          pctDone = (float)(ROUND_TIMER_HARD - roundTimer.getRemaining()) / (float)ROUND_TIMER_HARD;
//          pctDone *= 100.0;
//          if(pctDone >= 75){
//            myColor = 0;
//            flashAmt = FLASH_TIMER_MIN;
//          }
//          else if(pctDone >= 50){
//            myColor = 4;
//            flashAmt = FLASH_TIMER_MID;
//          }
//          else{
//            myColor = 2;            
//            flashAmt = FLASH_TIMER_MAX;
//          }
//        }
//      }
//  }
//
//  //Set Color
//  if(isFlasher && flashOff){
//    setColor(OFF);
//  }
//  else{
//    setColor(colors[myColor]);
//  }
//  
//}
