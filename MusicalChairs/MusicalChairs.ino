/*
 * SETUP: Create a 7-blink cluster (6 blinks in a ring, with one at the center) place one blink on each outer blink so that they only have one connection. (They will turn green)
 * When all players are ready, double click the center (cyan) blink.
 * 
 * GAMEPLAY: All players find the other player with their blink color and form a team. Going one at a time, they flip their blinks on the edge of the table.
 * When all players on a team have flipp\ed their blinks, connect back to the part of the ring matching their color. Anyone who does not make it back to the center before the timer is complete loses.
 */
#define PLAYER 1
#define CHANGESTATE 10
#define NEEDSCOLOR 21
#define IDLE_STATE 22
#define COLOR_RECEIVED 23
#define RESET 24
#define SETTINGCOLORPLAYER 40
#define SETTINGCOLORSPACES 50
#define STATE_TIMER 500
#define BLINK_TIMER_MIN 200
#define BLINK_TIMER_MID 400
#define BLINK_TIMER_MAX 600
#define VERIFY_ID_TIMER 100
#define ROUND_TIMER_EASY 30000
#define ROUND_TIMER_MEDIUM 20000
#define ROUND_TIMER_HARD 10000

Color colors[] = {RED, BLUE, GREEN, ORANGE, YELLOW, MAGENTA, WHITE, CYAN};
bool isPlayer = false;
bool isBrain = false;
Timer stateChangeTimer;
bool stateChangeComplete = false;

//Game logic
byte faceCount;
bool isResetting = false;

//Players
byte myColor = 6;
byte myIndex = 0;
bool colorAssigned = false;

//Brain
byte rnd;
byte currentColor;
byte foundCount;
bool generateNewColor = true;
bool assignColorsToPlayers = true;
bool assignColorsToSpaces = false;
byte color1 = 0;
byte color2 = 0;
byte color3 = 0;
Timer roundTimer;
bool roundTimerSet = false;
float pctDone;

//Ring
bool hasColor = false;
bool isRing = false;
bool isRingAnchor = false;

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

  buttonDoubleClicked();
}

void setupLoop() {
  //Determine who is a player and check for state change
   faceCount = 0;
   isRingAnchor = false;
   isRing = false;
   FOREACH_FACE(f){
    if(!isValueReceivedOnFaceExpired(f)){
      faceCount++;
      if(getLastValueReceivedOnFace(f) == PLAYER){
        isRingAnchor = true;
      }
      if(getLastValueReceivedOnFace(f) == CHANGESTATE-1 && !isRingAnchor){
        state = INGAME;
        isRing = true;
        setValueSentOnFace(CHANGESTATE-1, (f+3)%6);
      }
      if(getLastValueReceivedOnFace(f) >= CHANGESTATE){
        state = INGAME;
        if(!isPlayer){
          setValueSentOnFace(getLastValueReceivedOnFace(f), (f+3)%6);
          if(isRingAnchor){
            for(byte i = 0; i < 6; i++){
              if(i != f && i != (f+3)%6){
                setValueSentOnFace(CHANGESTATE-1, i);
              }
            }
          }  
        }
        if(isPlayer)
          myIndex = getLastValueReceivedOnFace(f) - CHANGESTATE;
      }
    }
   }

  //Check Players and Spinner
  isPlayer = faceCount <= 1;
  isBrain = faceCount == 6;

  //Change State
  if(buttonDoubleClicked() && isBrain){
    state = INGAME;
    FOREACH_FACE(f){
      setValueSentOnFace(CHANGESTATE+f,f);
    }
  }

  if(isPlayer){
    setValueSentOnAllFaces(PLAYER);
  }
  
  //Draw
  if(isPlayer)
    setColor(GREEN);
  else if(isBrain)
    setColor(CYAN);
  else
    setColor(WHITE);
}

void inGameLoop(){
  //Player
  if(isPlayer){ 
    faceCount = 0;
    FOREACH_FACE(f){
      if(!isValueReceivedOnFaceExpired(f)){
        faceCount++; 
        if(!colorAssigned){ //I need a color
          if(getLastValueReceivedOnFace(f) >= SETTINGCOLORPLAYER && getLastValueReceivedOnFace(f) < SETTINGCOLORSPACES){
           myColor = getLastValueReceivedOnFace(f) - SETTINGCOLORPLAYER; //Get my color from the ring
           isBlinker = false;
           colorAssigned = true;
           setValueSentOnAllFaces(COLOR_RECEIVED);
          }
        }
      }
    }
  }
  //BRAIN
  if(isBrain){
    myColor = 7;
    if(assignColorsToPlayers){
      byte assigned[] = {6,6,6,6,6,6};
      foundCount = 0;
      generateNewColor = true;
      //Generate new colors
      color1 = random(5);
      color2 = color1;
      while(color2 == color1){
        color2 = random(5);
      }
      color3 = color2;
      while(color3 == color2 || color3 == color1){
        color3 = random(5);
      }
      //Distribute colors along paths
      for(byte n = 0; n < 6; n++){
        currentColor = 6;
        generateNewColor = true;
        while(generateNewColor){
          foundCount = 0;
          rnd = random(2);
          switch(rnd){
            case 0:
              currentColor = color1;
              break;
            case 1:
              currentColor = color2;
              break;
            case 2:
              currentColor = color3;
              break; 
           }

           //Check if this color has already been assigned twice
           for(byte i = 0; i < 6; i++){
            if(assigned[i] == currentColor)
              foundCount++;
           }
            
           if(foundCount >= 2)
            generateNewColor = true;
           else{
            generateNewColor = false;
            assigned[n] = currentColor;
            setValueSentOnFace(SETTINGCOLORPLAYER+currentColor, n);
           }
          }
        }
        assignColorsToPlayers = false;
      }

      if(assignColorsToSpaces){
         byte assignedSpaces[] = {6,6,6};
         foundCount = 0;
         generateNewColor = true;
         //Pick our spots and colors
         for(byte n = 0; n < 6; n++){
          currentColor = 6;
          generateNewColor = true;
          while(generateNewColor){
            foundCount = 0;
            rnd = random(2);
            switch(rnd){
              case 0:
                currentColor = color1;
                break;
              case 1:
                currentColor = color2;
                break;
              case 2:
                currentColor = color3;
                break; 
            }

            //Check if this color has already been assigned twice
            for(byte i = 0; i < 6; i++){
              if(assignedSpaces[i] == currentColor)
                foundCount++;
            }
            
            if(foundCount >= 2)
              generateNewColor = true;
            else{
              generateNewColor = false;
              assignedSpaces[n] = currentColor;
              assignedSpaces[n+1] = currentColor;
              setValueSentOnFace(SETTINGCOLORSPACES+currentColor, n);
              setValueSentOnFace(SETTINGCOLORSPACES+currentColor, n+1);
              n++;
            }
          }
        }
        assignColorsToSpaces = false;
      }
      
      FOREACH_FACE(f){
        if(!isValueReceivedOnFaceExpired(f)){
          if(getLastValueReceivedOnFace(f) == NEEDSCOLOR && stateChangeTimer.isExpired()){
            assignColorsToSpaces = true;
            stateChangeTimer.set(1000);
          }
        }
      }
  }
  //RING
  if(!isPlayer && !isBrain && !isRing){
    FOREACH_FACE(f){
      if(!isValueReceivedOnFaceExpired(f)){
        if(getLastValueReceivedOnFace(f) >= SETTINGCOLORPLAYER && getLastValueReceivedOnFace(f) < SETTINGCOLORSPACES){
          setValueSentOnFace(getLastValueReceivedOnFace(f),(f+3)%6);
        }
        if(getLastValueReceivedOnFace(f) >= SETTINGCOLORSPACES && !hasColor){
          if(isRingAnchor && !hasColor){
            myColor = getLastValueReceivedOnFace(f) - SETTINGCOLORSPACES;
            setValueSentOnAllFaces(IDLE_STATE);
          }
          else{
            setValueSentOnFace(getLastValueReceivedOnFace(f), (f+3)%6);
          }
          hasColor = true;
        }
        if(getLastValueReceivedOnFace(f) == COLOR_RECEIVED && !hasColor){
          setValueSentOnFace(NEEDSCOLOR, (f+3)%6);
        } 
        if(getLastValueReceivedOnFace(f) == NEEDSCOLOR){
          setValueSentOnFace(NEEDSCOLOR, (f+3)%6);
        }
      }
    }
  }


  //Timer
  if(!roundTimerSet){
    roundTimer.set(ROUND_TIMER_HARD);
    roundTimerSet = true;
  }
  else{
    if(roundTimer.isExpired()){
        if(isBrain){
          isBlinker = false;
          }
        if(isPlayer){
          isBlinker = true;
          myColor = 0;
          FOREACH_FACE(f){
            if(!isValueReceivedOnFaceExpired(f)){
              if(getLastValueReceivedOnFace(f) == IDLE_STATE){
                myColor = 2;
              }
            }
          }
        }
      }
      else{
        if(isBrain){
          isBlinker = true;
          pctDone = (float)(ROUND_TIMER_HARD - roundTimer.getRemaining()) / (float)ROUND_TIMER_HARD;
          pctDone *= 100.0;
          if(pctDone >= 75){
            myColor = 0;
            blinkAmt = BLINK_TIMER_MIN;
          }
          else if(pctDone >= 50){
            myColor = 4;
            blinkAmt = BLINK_TIMER_MID;
          }
          else{
            myColor = 2;            
            blinkAmt = BLINK_TIMER_MAX;
          }
        }
      }
  }

  //Set Color
  blinkBlinkers();
  
  if(isBlinker && blinkOff){
    setColor(OFF);
  }
  else if(isRing){
    setColor(CYAN);
  }
  else{
    setColor(colors[myColor]);
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

void resetVars(){
  colorAssigned = false;
  assignColorsToPlayers = true;
  assignColorsToSpaces = false;
  roundTimerSet = false;
  hasColor = false;
  isBlinker = false;
  state = SETUP;
}
