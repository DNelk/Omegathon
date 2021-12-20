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
const byte RESET_2 = 63;

//Flashing
const byte           FLASH_TIMER_MIN = 200;
const unsigned int   FLASH_TIMER_MID = 400;
const unsigned int   FLASH_TIMER_MAX = 600;

//Timer Lengths
const unsigned int   SPIN_TIMER_MAX = 200;
const byte           SPIN_TIMER_START = 100;
const unsigned int   LAUNCH_NEW_GAME = 200;
const unsigned int   ROUND_LENGTH_CHEERS = 15000;
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


//Spinner
bool isSpinning; //Is the spinner currently running?
byte spinCurrent; //Current face the spinner is landed on
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
unsigned int pctDone;

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
  
  checkForReset();
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
  
  if(isPlayer || isBrain){
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
          state = GAME_SETUP;
          break;
        case 1:
          tempState = CHEERS;
          state = GAME_SETUP;
          break;
        case 2:
         tempState = CHAIRS;
         state = GAME_SETUP;
         break;
      }
      /*tempState = RLGL;
      state = GAME_SETUP;*/
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
  byte setupColor;
  byte setupSignal;
  //Set our color and send out signals depending on the game we are going to play.
  switch(tempState){
    case RLGL:
      setupColor = 0;
      setupSignal = LAUNCH_RLGL;
      rlglReset();
      break;
    case CHEERS:
      setupColor = 1;
      setupSignal = LAUNCH_CHEERS;
      cheersReset();
      break;
    case CHAIRS:
      setupColor = 2;
      setupSignal = LAUNCH_CHAIRS;
      chairsReset();
      break;
  }

  if(!isPlayer){
    setColor(gameColors[setupColor]);
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
    if(isSpoke){
      setColor(OFF);
    }
  }

}


///
//RLGL
///

const unsigned int RED_INTERVAL_MAX = 4000;
const unsigned int RED_INTERVAL_MIN = 2000;

const unsigned int GREEN_INTERVAL_MAX = 4000;
const unsigned int GREEN_INTERVAL_MIN = 2000;

const unsigned int RIPPLING_INTERVAL = 300;
const byte CLICKTHRESHOLD = 10;

const byte RED_LIGHT = 41;
const byte GREEN_LIGHT = 42;

bool isGreenLight;

void rlglReset(){
  isGreenLight = false;  //check if light is green or red
  roundOver = true; //checks if the light changes in order to set timers
  gameOver = false;
  launchingGame = false;
  score = 0;
  if(isPlayer){
    FOREACH_FACE(f){
      if(!isValueReceivedOnFaceExpired(f)){
        brainFace = f;
        break;
      }
    }
  }  
}

void rlglGameLoop() {
  
 if(!gameOver){   
  //Brain behavior - red or green light
  if(isBrain){    
    if(isGreenLight){ //Green light!
      setValueSentOnAllFaces(GREEN_LIGHT);
      
      //Set the round timer
      if(roundOver){
        roundTimer.set(GREEN_INTERVAL_MIN + random(GREEN_INTERVAL_MAX));
        roundOver = false;
      }

      //Set Color
      if(roundTimer.getRemaining() < RIPPLING_INTERVAL){
        flashColor(gameColors[3]);
      }
      else{
        setColor(gameColors[2]);
      }
     
    }
    else{ //Red light!
      setValueSentOnAllFaces(RED_LIGHT);

      //Set timer
      if(roundOver){
        roundTimer.set(RED_INTERVAL_MIN + random(RED_INTERVAL_MAX));
        roundOver = false;
      }

      //Set color
      setColor(RED);
    }

   //TIME'S UP!
    if(roundTimer.isExpired()){
      roundOver = true;
      isGreenLight = !isGreenLight;
    }    
  }

  //Player behavior
  if(isPlayer){
    isGreenLight = getLastValueReceivedOnFace(brainFace) == GREEN_LIGHT;
    //Gaining points
    if(isGreenLight && !isAlone()){
      if(getLastValueReceivedOnFace(brainFace) == RED_LIGHT){
        isGreenLight = false;
      }
      else if(buttonPressed()){
        score++;
        /*while(score <= RESET && score >= WIN){
          score++;
        }*/
        if(score >= CLICKTHRESHOLD * 4){
          gameOver = true;
          setValueSentOnAllFaces(WIN);
          setColor(gameColors[2]);
        }
      }
    }
    else{ //Losing Points
      if(buttonPressed()){
        score = 0;
      }
    }
    if(!gameOver){
      setValueSentOnAllFaces(score);
    }
  }

  if(isSpoke){
    byte playerReceived = getLastValueReceivedOnFace(playerFace);
    byte brainReceived = getLastValueReceivedOnFace(brainFace);
    
    //Communication
   if(brainReceived == RED_LIGHT || brainReceived == GREEN_LIGHT){
      passToBrain(playerReceived);
      passToPlayer(brainReceived);
      //Set Color
      setColor(dim(gameColors[myID], min(255, (playerReceived * 255)/CLICKTHRESHOLD)));
      byte tempScore = max(playerReceived - CLICKTHRESHOLD, 0);
      /*while(tempScore <= RESET && tempScore >= WIN){
        tempScore++;
      }*/
      setValueSentOnFace(tempScore,brainFace);
    }
  }

  if((checkAllFacesForValue(WIN) || checkAllFacesForValue(LOSE))&& !gameOver){
    gameOver = true;
    setColor(dim(gameColors[0], 100));
    setValueSentOnAllFaces(LOSE);
  }
 }
 else{
  if(isBrain && buttonDoubleClicked()){
    state = BOARD_IDLE;
    setValueSentOnAllFaces(BUILD_BOARD);
  }
  if(!isBrain){
    if(checkAllFacesForValue(BUILD_BOARD)){
      state = BOARD_IDLE;
      setValueSentOnAllFaces(BUILD_BOARD);
    }
  }
 }
 
}
///
//CHEERS!
///

byte scores[6];
byte highestScore;
bool allowBump;
byte lastBumped;
byte receivedCount;

void cheersReset(){
  score = 0;
  allowBump = false;
  lastBumped = 0;
  roundTimerStarted = false;  
  launchingGame = false;
  receivedCount = 0;
  for(byte i = 0; i < 6; i++){
    scores[i] = 0;
  }
  highestScore = 0;
}

void cheersGameLoop(){
  //Timer
  if(!roundTimerStarted){ //Timer needs to be set
    roundTimer.set(ROUND_LENGTH_CHEERS);
    roundTimerStarted = true;
  }
  
  //PLAYER - Allow bumps with other players
  if(isPlayer){ 
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
      setValueSentOnAllFaces(score);
      if(isAlone()){
        setColor(gameColors[myID]);
      }
    }
  }
  
  //SPINNER AND RING - Display timer
  if(!isPlayer){
    if(roundTimerStarted){
     if(!roundTimer.isExpired()){ //Timer has not expired yet
      pctDone = ROUND_LENGTH_CHEERS - roundTimer.getRemaining();
      pctDone = pctDone * 100;
      pctDone = pctDone / ROUND_LENGTH_CHEERS;
      if(pctDone >= 75){
        flashAmt = FLASH_TIMER_MIN;
        flashColor(RED);
      }
      else if(pctDone >= 50){
        flashAmt = FLASH_TIMER_MID;
        flashColor(YELLOW);
      }
      else{
        flashAmt = FLASH_TIMER_MAX;
        flashColor(GREEN);
      }
     }
     else{ //Timer is expired
      setColor(RED);
     }
    }
  }

   //Brain- send out winners/losers
   if(isBrain && roundTimerStarted && roundTimer.isExpired()){
    byte receivedCount = 0;
    FOREACH_FACE(f){ //Get Scores
      if(getLastValueReceivedOnFace(f) != LAUNCH_CHEERS){
        scores[f] = getLastValueReceivedOnFace(f);
        receivedCount++;
        if(scores[f] > highestScore){
          highestScore = scores[f];
        }
      }
    }
    if(receivedCount == 5){
      FOREACH_FACE(f){
        if(scores[f] == highestScore){
          setValueSentOnFace(WIN, f);
        }
        else{
          setValueSentOnFace(NONE,f);
        }
      }
    }
    if(buttonDoubleClicked()){ //Reset
      state = BOARD_IDLE;
      setValueSentOnAllFaces(BUILD_BOARD);
    }
  }
   
  if(!isBrain && roundTimerStarted && roundTimer.isExpired()){
    //Timer expired, let's send signals and resolve this game
    if(isSpoke){
      setValueSentOnFace(getLastValueReceivedOnFace(brainFace),playerFace);
      setValueSentOnFace(getLastValueReceivedOnFace(playerFace),brainFace);
    }
 
    if(checkAllFacesForValue(BUILD_BOARD)){
      state = BOARD_IDLE;
      setValueSentOnAllFaces(BUILD_BOARD);
    }
  }
}

///
//FLIPSWITCH
///
byte matchColor;

void chairsReset(){
  gameOver = false;
  if(isSpoke){
    matchColor = normalizeFace(myID + 3);
  }
  launchingGame = false;
}

void chairsGameLoop(){
  if(gameOver || isGameBoard){
    if(isBrain && buttonDoubleClicked()){
      state = BOARD_IDLE;
      setValueSentOnAllFaces(BUILD_BOARD);
    }
    if(!isBrain){
      if(checkAllFacesForValue(BUILD_BOARD)){
        state = BOARD_IDLE;
        setValueSentOnAllFaces(BUILD_BOARD);
      }
    }
    return;
  }
  //Generate 3 color pairs
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
            }
            break;
          }
        }
      }
    }
  }

  //Players need to attach to their matching color
  if(isPlayer){
    if(isAlone()){
      setValueSentOnAllFaces(SET_ID + myID);
    }
  }
  else if(isSpoke){
    if(getLastValueReceivedOnFace(brainFace) == LOSE){
      passToPlayer(LOSE);
      setColor(RED);
      gameOver = true;
    }
    else if(getLastValueReceivedOnFace(playerFace) == WIN){
      passToBrain(WIN);
      setColor(GREEN);
      gameOver = true;
    }

    if(isJoint){
      if(getLastValueReceivedOnFace(playerFace) - SET_ID == matchColor){
        setValueSentOnAllFaces(WIN);
        setColor(GREEN);
        gameOver = true;
      }
    }
  }
  
  if((isPlayer || isSpoke)){
    winLoseCheck();
    if(!gameOver){
      if(isPlayer){
        setColor(gameColors[myID]);
      }
      else{
        setColor(gameColors[matchColor]);
      }
    }   
  }
}

void winLoseCheck(){
  if(checkAllFacesForValue(LOSE) && !gameOver){
    setColor(gameColors[0]);
    gameOver = true;
  }
  if(checkAllFacesForValue(WIN) && !gameOver){
    setColor(gameColors[2]);
    gameOver = true;
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

byte faceCount;
//Check if this is a player (1 or fewer connections) or the brain (6 connections)
void checkIfPlayerOrBrain(){
  faceCount = 0;
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
  
  if(!isBrain){
    FOREACH_FACE(f){
      if(!isValueReceivedOnFaceExpired(f)){
        if(getLastValueReceivedOnFace(f) >= RESET_1){
          if(didValueOnFaceChange(f)){
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
  spinCurrent = random(2);
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
