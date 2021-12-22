/*Constants*/

const byte PLAYER = 6; //Default "Player" signal
const byte SPIN_TARGET = 7; //Communicates if the spinner is on a spoke
const byte BUILD_BOARD = 9; //Used for resetting the board after a game
const byte SET_ID = 10; //Used in conjuction with an ID to communicate ID
const byte LAUNCH_RLGL = 16;    ///
const byte LAUNCH_CHEERS = 17;  //Signals for launching games
const byte LAUNCH_CHAIRS = 18;  ///
const byte START_GAME = 19; //Signal for launching a game in general
const byte SET_BOARD = 20; //Signal for creating the board ring
const byte WIN = 59;  //Signals for winning and losing
const byte LOSE = 60; // ^^^
const byte NONE = 61; //Basically a blank signal 
const byte RESET_1 = 62; //Signals for resetting the board
const byte RESET_2 = 63; // ^^^

//Flashing
const byte           FLASH_TIMER_MIN = 200;
const unsigned int   FLASH_TIMER_MID = 400;
const unsigned int   FLASH_TIMER_MAX = 600;

//Timer Lengths
const unsigned int   SPIN_TIMER_MAX = 200;
const byte           SPIN_TIMER_START = 100;
const unsigned int   LAUNCH_NEW_GAME = 200;
const unsigned int   ROUND_LENGTH_CHEERS = 10000;
const unsigned int   GAME_START = 500;

/*Vars*/

//Colors
const Color gameColors[] = {RED,BLUE,GREEN,YELLOW,ORANGE,MAGENTA,WHITE,CYAN,OFF};

//Roles
bool isPlayer; //Is this one of 6 players
bool isBrain; //Is this the central piece (this piece does lots of calculations)
bool isSpoke; //Is this one of the pieces that connects to the spinner ring and the joints
bool isJoint; //Is this the spoke piece that touches a player?
bool isGameBoard; //Is this the ring where players put their pieces?
byte brainFace; //The face that goes toward the center
byte playerFace; //The face that goes toward the outside
byte resetSignal = RESET_1;
//byte playersConnected;


//Spinner
bool isSpinning; //Is the spinner currently running?
byte spinCurrent; //Current face the spinner is landed on
byte lastSpin; //Last thing we landed on
bool launchingGame; //Are we launching a game right now?
unsigned int currentTimeAmount; //Current spinner interval length



//"Global" Vars
byte myID; //Id of this piece
byte score; //Used where score is relevant (like Red Light Green Light and Cheers)
bool gameOver; //Used when the game is over
bool roundOver; //Used when the round is over (like in RLGL)
//bool allowReset;

//Game State
enum GameState{
  BOARD_SETUP, //Assigning roles to all the pieces of the board
  BOARD_IDLE, //Waiting to play a game
  RLGL, //Playing red light green light
  CHEERS, //Playing Cheers
  CHAIRS, //Playing musical chairs
  GAME_SETUP //Setup State
};

GameState state; //Current game state
GameState tempState; //Temp game state (for switching between games)

//Flashing
Timer flashTimer;
unsigned int flashAmt;

//Timer
Timer roundTimer;
bool roundTimerStarted;

//*Functions*/

//Initialize variables
void init(){
  isPlayer = false;
  isBrain = false;
  isSpoke = false;
  isJoint = false;
  isGameBoard = true;
  isSpinning = false;
  launchingGame = false;
  myID = 6;
  score = 0;
  state = BOARD_SETUP;
  brainFace = playerFace = 0;
  flashAmt = FLASH_TIMER_MIN;
}

void setup() {
  // put your setup code here, to run once:
  randomize();
  init();
}

void loop() {

  checkForReset();
    
  // put your main code here, to run repeatedly:
  switch(state){
    case BOARD_SETUP:
      boardSetup();
      break;
    case BOARD_IDLE:
      boardLoop();
      break;
    case GAME_SETUP:
      gameSetup();
      break;
    case RLGL:
      rlglGameLoop();
      break;
    case CHEERS:
      cheersGameLoop();
      break;
    case CHAIRS:
      chairsGameLoop();
      break;
  }
}

//Assign our roles
void boardSetup(){
  checkIfPlayerOrBrain(); //Figure out if we're a player or brain

  //Send out/recieve signals to assign roles
  if(!isBrain){
    FOREACH_FACE(f){
      if(!isValueReceivedOnFaceExpired(f)){
        byte received = getLastValueReceivedOnFace(f);
        
        //Did we recieve a SET_ID signal from the brain/previous spoke
        if(received >= SET_ID && received < SET_ID + 6){
          
          //Isolate my ID, determine my player and brain facing faces
          myID = received - SET_ID;
          brainFace = f;
          playerFace = normalizeFace(f+3);
          isGameBoard = false;

          //Set spoke and joint
          if(!isPlayer){
            isSpoke = true;
            isPlayer = false;
            isBrain = false;
            if(isValueReceivedOnFaceExpired(playerFace) || getLastValueReceivedOnFace(playerFace) == PLAYER){
              isJoint = true;
              setValueSentOnFace(((myID+1)%3) + SET_BOARD, nextFaceCounterClockwise(f));
            }
          }

          //Send signal, change state
          passToPlayer(getLastValueReceivedOnFace(f));
          state = BOARD_IDLE;
          break;
        } 
        else if(received >= SET_BOARD && received < NONE  && didValueOnFaceChange(f)){
          //If we received a SET_BOARD Signal, we are a game board piece.
          myID = received - SET_BOARD;
          setValueSentOnFace(((myID+1)%3) + SET_BOARD, normalizeFace(f+3));
          state = BOARD_IDLE;
          isSpoke = false;
          isPlayer = false;
          isBrain = false;
          break;
        }
      }
    }
  }
  else{ //Brain--Send initial signals
    if(buttonDoubleClicked()){
      FOREACH_FACE(f){
        setValueSentOnFace(SET_ID + f,f);
      }
      state = BOARD_IDLE;
      isGameBoard = false;
      myID = 7;
    }
  }

  //Draw
  if(isBrain){
    setColor(gameColors[5]);
  }
  else if(isPlayer){
    setColor(gameColors[0]);
  }
  else{
    setColor(gameColors[3]);
  }
}

//This is the loop for the board_idle state
void boardLoop(){
  //Draw
  if(isGameBoard){
    setColor(gameColors[myID%3]);
  }
  else if(isPlayer || isBrain){
    setColor(gameColors[myID]);
  }

  //Spokes - spinner coloring
  if(isSpoke){
    if(getLastValueReceivedOnFace(brainFace) == SPIN_TARGET){
      setColor(gameColors[myID%3]);
      if(!isJoint){
        passToPlayer(SPIN_TARGET);
      }
    }
    else if(getLastValueReceivedOnFace(brainFace) == NONE){
      if(!isJoint){
        passToPlayer(NONE);
      }
      setColor(WHITE);
    }
    else if(isJoint){
      flashColor(gameColors[myID%3]);
      setColorOnFace(gameColors[myID], playerFace);
    }
    else{
      setColor(WHITE);
    }
  }

  if(isBrain){
    //Input-- spin the spinner
    if(buttonDoubleClicked() && !isSpinning){
      startSpinner();
    }
    else if(isSpinning){ //Spin the Spinner
      spinSpinner();
    }
    else if(launchingGame && roundTimer.isExpired()){//Check for game launch
      switch((spinCurrent % 3)){
       case 0:
          tempState = RLGL;
          break;
        case 1:
          tempState = CHEERS;
          break;
        case 2:
         tempState = CHAIRS;
         break;
        default:
          tempState = RLGL;
      }
      state = GAME_SETUP;
    }
  }
  else{
    //Check for state change!
    if(checkAllFacesForValue(LAUNCH_RLGL)){
      tempState = RLGL;
      state = GAME_SETUP;
    }
    else if(checkAllFacesForValue(LAUNCH_CHEERS)){
      tempState = CHEERS;
      state = GAME_SETUP;  
    }
    else if(checkAllFacesForValue(LAUNCH_CHAIRS)){
      tempState = CHAIRS;
      state = GAME_SETUP;
    }
  }
}

/*GAMES*/

///
//GAME SETUP
///
void gameSetup(){
  launchingGame = false;
  resetAllGames();
  
  byte setupColor;
  byte setupSignal;
  //Set our color and send out signals depending on the game we are going to play.
  switch(tempState){
    case RLGL:
      setupColor = 0;
      setupSignal = LAUNCH_RLGL;
      break;
    case CHEERS:
      setupColor = 1;
      setupSignal = LAUNCH_CHEERS;
      break;
    case CHAIRS:
      setupColor = 2;
      setupSignal = LAUNCH_CHAIRS;
      break;
    default:
      setupColor = 0;
      setupSignal = LAUNCH_RLGL;
  }

  if(!isPlayer){
    setColor(gameColors[setupColor]);
  }
  else{
    flashColor(gameColors[myID]);
  }
  setValueSentOnAllFaces(setupSignal);
  
  //Change State Input
  if(buttonDoubleClicked() && isBrain){
    state = tempState;
    setValueSentOnAllFaces(START_GAME);
  }

  //Check for state change!
  if(!isBrain && checkAllFacesForValue(START_GAME)){
    state = tempState;
    
    setValueSentOnAllFaces(START_GAME);
    if(isGameBoard){
      setColor(WHITE);
    }
    else if(isSpoke){
      setColor(OFF);
    }
    else if(isPlayer){
      setColor(gameColors[myID]);
      FOREACH_FACE(f){
        if(!isValueReceivedOnFaceExpired(f)){
          brainFace = f;
          break;
        }
      }
    }  
  }
}



///
//RLGL
///

const unsigned int LIGHT_INTERVAL_MAX = 4000;
const unsigned int LIGHT_INTERVAL_MIN = 2000;

const unsigned int RIPPLING_INTERVAL = 500;
const byte CLICKTHRESHOLD = 10;

const byte RED_LIGHT = 41;
const byte GREEN_LIGHT = 42;

bool isGreenLight;

void rlglGameLoop() {
  
 if(!gameOver){   
  //Brain behavior - red or green light
  if(isBrain){  
    //Set the round timer
    if(roundTimer.isExpired()){//TIME'S UP!
      roundTimer.set(LIGHT_INTERVAL_MIN + random(LIGHT_INTERVAL_MAX));
      isGreenLight = !isGreenLight;
      if(isGreenLight){
        setValueSentOnAllFaces(GREEN_LIGHT);
        setColor(gameColors[2]); //Set to green
      }
      else{
        setValueSentOnAllFaces(RED_LIGHT);
        setColor(gameColors[0]); //Set to red
      }
    }
    else if(isGreenLight){ //Green light!
      //Set Color
      if(roundTimer.getRemaining() < RIPPLING_INTERVAL){
        flashColor(gameColors[3]); //Flicker to yellow 
      }
    } 
  }
  else if(isPlayer){ //Player behavior
    byte brainReceived = getLastValueReceivedOnFace(brainFace);
    isGreenLight = brainReceived == GREEN_LIGHT;
    if(buttonPressed()){
      //Gaining points
      if(isGreenLight){
        score++;
        if(score >= CLICKTHRESHOLD * 4){
          gameOver = true;
          setValueSentOnAllFaces(WIN);
          setColor(gameColors[2]);
        }
      }
      else{ //Losing Points
        score = 0;
      }
    }
    if(!gameOver){
      setValueSentOnAllFaces(score);
    }
  }
  else if(isSpoke){
    byte playerReceived = getLastValueReceivedOnFace(playerFace);
    byte brainReceived = getLastValueReceivedOnFace(brainFace);
    
    //Communication
   if(brainReceived == RED_LIGHT || brainReceived == GREEN_LIGHT){
      passToBrain(playerReceived);
      passToPlayer(brainReceived);
      //Set Color
      setColor(dim(gameColors[myID], min(255, (playerReceived * 255)/CLICKTHRESHOLD)));
      byte tempScore = max(playerReceived - CLICKTHRESHOLD, 0);
      setValueSentOnFace(tempScore,brainFace);
    }
  }

  if(checkAllFacesForValue(WIN) && !gameOver){
    gameOver = true;
    setColor(dim(gameColors[0], 100));
    setValueSentOnAllFaces(WIN);
  }
 }
 else{ //Game over!
  checkIfRebuildBoard();
 }
 
}
///
//CHEERS!
///

byte winnerFace;
byte highestScore;
bool allowBump;
byte lastBumped;
byte receivedCount;

void cheersGameLoop(){
  if(!gameOver){
    //Timer
    if(!roundTimerStarted){ //Timer needs to be set
      roundTimer.set(ROUND_LENGTH_CHEERS);
      roundTimerStarted = true;
    } 
    else if(isPlayer){ //PLAYER - Allow bumps with other players
      if(!roundTimer.isExpired()){
        if(isAlone()){ //Check number of faces to see if we're alone. If we are, we allow bump
          allowBump = true;
          flashColor(gameColors[myID]);
          setValueSentOnAllFaces(SET_ID + myID);
        }
        else if(allowBump){ //ID check for when we're not alone
          allowBump = false;
          FOREACH_FACE(f){
            if(!isValueReceivedOnFaceExpired(f)){
              if(getLastValueReceivedOnFace(f) != lastBumped && getLastValueReceivedOnFace(f) >= SET_ID){
                score++;
                lastBumped = getLastValueReceivedOnFace(f);
                break;
              }
            }
          }
        }
        else{ //Not alone but also not doing an id Check
          setColor(gameColors[myID]);
          setValueSentOnAllFaces(score);
        }
      }
      else{ //Time's up!
        if(checkAllFacesForValue(WIN)){
          setColor(gameColors[2]);
          gameOver = true;
        }
        else if(checkAllFacesForValue(LOSE)){
          setColor(gameColors[0]);
          gameOver = true;
        }
        else{
          setValueSentOnAllFaces(score); 
          setColor(gameColors[myID]);
        }
      }
    }
    else{//Display timer
      if(roundTimerStarted){
       if(!roundTimer.isExpired()){ //Timer has not expired yet
        unsigned int amtDone = roundTimer.getRemaining();
        if(amtDone < (ROUND_LENGTH_CHEERS/4)){
          flashAmt = FLASH_TIMER_MIN;
          flashColor(gameColors[0]);
        }
        else if(amtDone < (ROUND_LENGTH_CHEERS/2)){
          flashAmt = FLASH_TIMER_MID;
          flashColor(gameColors[3]);
          if(isSpoke){passToBrain(NONE);}
        }
        else{
          flashAmt = FLASH_TIMER_MAX;
          flashColor(gameColors[2]);
        }
       }
       else{ //Timer is expired
          setColor(dim(gameColors[0],100));
         if(isGameBoard){ //needs no logic
          gameOver = true;
         }
         else if(isBrain){ //Brain- send out winners/losers
          byte receivedCount = 0;
          byte received = 0;
          //setColor(OFF);
          FOREACH_FACE(f){ //Get Scores
            received = getLastValueReceivedOnFace(f);
            if(received != NONE){
              receivedCount++;
              if(received > highestScore){
                highestScore = received;
                winnerFace = f;
              }
            }
          }
          if(receivedCount == 5){
            FOREACH_FACE(f){
              if(f == winnerFace){
                setValueSentOnFace(WIN, f);
              }
              else{
                setValueSentOnFace(LOSE,f);
              }
              gameOver = true;
            }
          }
        }
        else if(isSpoke){ //Timer expired, let's send signals and resolve this game
          byte fromBrain = getLastValueReceivedOnFace(brainFace);
          byte fromPlayer = getLastValueReceivedOnFace(playerFace);
          if(fromBrain == WIN){
            setColor(gameColors[2]);
            gameOver = true;
            passToPlayer(fromBrain);
          }
          else if(fromBrain == LOSE){
            setColor(gameColors[0]);
            gameOver = true;
            passToPlayer(fromBrain);
          }
          else{
            if(!isValueReceivedOnFaceExpired(playerFace)){
             passToBrain(fromPlayer);
            }
            else{
              passToBrain(NONE);
            }
          }
        }
       }
      }
    }
  }
  else{
    checkIfRebuildBoard();
  }
}

///
//FLIPSWITCH
///
byte matchColor;

void chairsGameLoop(){
  if(gameOver || isGameBoard){ //Game Over
    checkIfRebuildBoard(); 
  }
  else{
    //Generate color pairs
    if(isBrain){
      FOREACH_FACE(f){
        if(!isValueReceivedOnFaceExpired(f)){
          if(getLastValueReceivedOnFace(f) == WIN){
            if(getLastValueReceivedOnFace(normalizeFace(f+3)) == WIN){
              gameOver = true;
              for(byte i = 0; i < 6; i++){
                if(i != f && i != normalizeFace(f+3)){
                 setValueSentOnFace(LOSE, i);
                }
                else{
                 setValueSentOnFace(WIN, i);
                }
              }
              break;
            }
          }
        }
      }
    }
    else if(isPlayer){ //Players need to attach to their matching color
      if(isAlone()){
        setValueSentOnAllFaces(SET_ID + myID);
        setColor(gameColors[myID]);
      }
      else if(checkAllFacesForValue(LOSE)){
        gameOver = true;
        setColor(gameColors[0]);
      }
      else if(checkAllFacesForValue(WIN)){
        setColor(gameColors[2]);
        gameOver = true;
      }
      else{
        setColor(gameColors[myID]);
      }
    }
    else if(isSpoke){
      byte playerReceived = getLastValueReceivedOnFace(playerFace);
      byte brainReceived = getLastValueReceivedOnFace(brainFace);
      
      if(brainReceived == LOSE){
        passToPlayer(LOSE);
        setColor(gameColors[0]);
        gameOver = true;
      }
      else if(brainReceived == WIN){
        passToPlayer(WIN);
        setColor(gameColors[2]);
        gameOver = true;
      }
      else if(playerReceived == WIN){
        passToBrain(WIN);
        setColor(gameColors[2]);
      }
      else if(isJoint && playerReceived - SET_ID == matchColor){
        passToBrain(WIN);
        setColor(gameColors[2]);
      }
      else{
        setColor(gameColors[matchColor]);
      }
    }
  }
}

/*Helpers*/

// Bring f down to 0<=f<FACE_COUNT
// Assumes that f<((FACE_COUNT*2)-1)
byte normalizeFace( byte f ) {
  if (f>=FACE_COUNT) {
    f-=FACE_COUNT;
  }
  return f;
}

byte nextFaceClockwise( byte f ) {
  return normalizeFace( f+1 );
}

byte nextFaceCounterClockwise( byte f ) {
  return normalizeFace( f + FACE_COUNT - 1 );
}

//Check if this is a player (1 or fewer connections) or the brain (6 connections)
void checkIfPlayerOrBrain(){
  byte faceCount = 0;
  FOREACH_FACE(f){
    if(!isValueReceivedOnFaceExpired(f)){
      faceCount++;
    }
  }
  
  if(isPlayer && faceCount > 1){
    setValueSentOnAllFaces(NONE);
  }
  
  isPlayer = faceCount <= 1;
  isBrain = faceCount == 6;

  if(isPlayer){
    setValueSentOnAllFaces(PLAYER);
  }
}

//Check for a reset
void checkForReset(){
  if(isBrain && buttonMultiClicked()){
    setValueSentOnAllFaces(resetSignal);
    init();
    if(resetSignal == RESET_1){
      resetSignal = RESET_2;
    }
    else{
      resetSignal = RESET_1;
    }
  }
  else if(!isBrain){
    FOREACH_FACE(f){
      if(!isValueReceivedOnFaceExpired(f)){
        if(getLastValueReceivedOnFace(f) >= RESET_1){
         /*if(getLastValueReceivedOnFace(f) == RESET_1)
            setColor(CYAN);
          else if(getLastValueReceivedOnFace(f) == RESET_2)
            setColor(MAGENTA);
          else
            setColor(WHITE);*/
          if(didValueOnFaceChange(f)){
            //setColor(ORANGE);
            setValueSentOnAllFaces(getLastValueReceivedOnFace(f));
            init();
            break;
          }
        }
      }
    }
  }
}

//Makes blinks flash from their color to black
bool flashOn;
void flashColor(Color defaultColor){
  if(flashTimer.isExpired()){
    flashTimer.set(flashAmt);
    flashOn = !flashOn;
  }
  if(!flashOn){
    setColor(defaultColor);
  }
  else{
    setColor(dim(defaultColor,100));
  }
}

//Start spinning the spinner
void startSpinner(){
  isSpinning = true;
  myID = 8;
  //while (spinCurrent == lastSpin){
    spinCurrent = random(2);
  //}
  //lastSpin = spinCurrent;
  roundTimer.set(SPIN_TIMER_START);
  currentTimeAmount = SPIN_TIMER_START;
}

//Spin the spinner!
void spinSpinner(){
  if(roundTimer.isExpired()){ //This interval is complete
    setValueSentOnFace(NONE, spinCurrent);
    if(currentTimeAmount < SPIN_TIMER_MAX){ //Is the spinner interval less than our max interval?
      currentTimeAmount = (currentTimeAmount*11)/10;
      roundTimer.set(currentTimeAmount);        
      spinCurrent = nextFaceClockwise(spinCurrent);
      setValueSentOnFace(SPIN_TARGET, spinCurrent);  
    }
    else{ //We are done spinning
      isSpinning = false;
      roundTimer.set(LAUNCH_NEW_GAME);
      launchingGame = true;
      myID = spinCurrent % 3;
    }
  }
  else{ //The interval is ongoing
    setColorOnFace(gameColors[spinCurrent%3], spinCurrent);
  }
}


//Pass info to the player face
void passToPlayer(byte n){
  setValueSentOnFace(n, playerFace);
}

//Pass info to the brain face
void passToBrain(byte n){
  setValueSentOnFace(n, brainFace);
}

//Loop through with foreach face and stop when a value is found
bool checkAllFacesForValue(byte n){
  FOREACH_FACE(f){
    if(!isValueReceivedOnFaceExpired(f)){
      if(getLastValueReceivedOnFace(f) == n){
        return true;
      }
    }
  }

  return false;
}

void checkIfRebuildBoard(){
  if(isBrain && buttonDoubleClicked()){
    state = BOARD_IDLE;
    setValueSentOnAllFaces(BUILD_BOARD);
  }
  else if(!isBrain){
    if(checkAllFacesForValue(BUILD_BOARD)){
      state = BOARD_IDLE;
      setValueSentOnAllFaces(BUILD_BOARD);
    }
  }
  flashAmt = FLASH_TIMER_MIN;
}

//Reset all games
void resetAllGames(){
  isGreenLight = false;
  gameOver = false;
  score = 0;
  allowBump = false;
  lastBumped = 0;
  roundTimerStarted = false;  
  receivedCount = 0;
  highestScore = 0;
  if(isSpoke){
    matchColor = normalizeFace(myID + 3);
  }
}
