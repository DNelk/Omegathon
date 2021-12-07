//Constants - Used for communication and timers
#define PLAYER 6
#define SPOKE 7
#define JOINT 8
#define SPIN_TARGET 9
#define SET_SPINNER 10
#define LAUNCH_RLGL 16
#define LAUNCH_CHEERS 17
#define LAUNCH_CHAIRS 18
#define START_GAME 19
#define SET_ID 20
#define BUILD_BOARD 30
#define WIN 60
#define LOSE 61
#define NONE 62
const unsigned int   SPIN_TIMER_MAX = 120;
const byte           SPIN_TIMER_START = 100;
const unsigned int   LAUNCH_NEW_GAME = 500;
const unsigned int   ROUND_LENGTH_CHEERS = 5000;
const byte           FLASH_TIMER_MIN = 200;
const unsigned int   FLASH_TIMER_MID = 400;
const unsigned int   FLASH_TIMER_MAX = 600;

//Data types
struct vector{
  byte a;
  byte b;
};

//Colors
const Color gameColors[] = {RED,BLUE,GREEN,YELLOW,ORANGE,MAGENTA,WHITE,CYAN,OFF};

//Roles
bool isPlayer; //Is this one of 6 players
bool isBrain; //Is this the central piece (this piece does lots of calculations)
bool isSpinnerRing; //Is this one of the pieces that surrounds the brain
bool isSpoke; //Is this one of the pieces that connects to the spinner ring and the joints
bool isJoint; //Is this one of the pieces that connects to the spokes and the players
bool isGameBoard; //Is this one of the external rings (these have colors so the game can be played)

//Player Vars
byte myID;
byte colorIndex;

//Round Timer
Timer roundTimer;
bool roundTimerStarted;

//Spinner
const byte spins[] = {0,1,2};
byte spinIndex;
bool isSpinning;
byte spinCurrent;
unsigned int currentTimeAmount;
bool launchingGame;

//Flashing
Timer flashTimer;
unsigned int flashAmt;


//CHEERS
//Players
bool allowBump; //Allow a player to player bump?
byte lastBumped;
byte bumpCount;
byte cheersPlayerHue; //Color hue
//Brain
byte receivedCount;
byte receivedPlayers[6];
//Timer
unsigned long pctDone; //how close is the timer to being done?

enum GameState{
  BOARD_SETUP, //Assigning roles to all the pieces of the board
  BOARD_IDLE, //Waiting to play a game
  RLGL_SETUP, //Red light green light setup
  RLGL_GAME, //Playing red light green light
  CHEERS_SETUP, //Cheers setup
  CHEERS_GAME, //Playing Cheers
  CHAIRS_SETUP, //Musical chairs setup
  CHAIRS_GAME //Playing Musical chairs 
};

GameState state; //Current game state

//Initialize our variables
void init(){
  isPlayer = false;
  isBrain = false;
  isSpinnerRing = false;
  isGameBoard = false;
  isJoint = false;
  isSpoke = false;
  state = BOARD_SETUP;
  myID = 0;
  colorIndex = 6;
  roundTimerStarted = false;
  spinIndex = 0;
  isSpinning = false;
  flashAmt = FLASH_TIMER_MIN;
  launchingGame = false;
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
      boardSetupLoop();
      break;
    case BOARD_IDLE:
      boardLoop();
      break;
    case RLGL_SETUP:
      break;
    case CHEERS_SETUP:
      setValueSentOnAllFaces(LAUNCH_CHEERS);
      cheersSetupLoop();
      break;
    case CHEERS_GAME:
      cheersGameLoop();
      break;
    case CHAIRS_SETUP:
      setValueSentOnAllFaces(LAUNCH_CHAIRS);
      chairsSetupLoop();
      break;
    case CHAIRS_GAME:
      chairsGameLoop();
      break;
  }
}

//Assign our roles
void boardSetupLoop(){
  checkIfPlayerOrBrain(); //Figure out if we're a player or brain

  //Send out some initial signals
  if(isPlayer){
    setValueSentOnAllFaces(PLAYER);
  }
  else{
    setValueSentOnAllFaces(0);
  }

  //Set Default Colors
  if(isBrain){
    colorIndex = 7;
  }
  else if(isPlayer && colorIndex == 6){
    colorIndex = 0;
  }
  else if(!isPlayer){
    colorIndex = 6;
  }

  //State change! Create the spinner ring and game ring, assign ids to players, set the state
  if(buttonDoubleClicked() && isBrain){
    FOREACH_FACE(f){
      setValueSentOnFace(SET_SPINNER+f,f);
    }
    state = BOARD_IDLE;
  }

  //Check for state change!  
  if(!isBrain){
    checkBoardStateChange();
  }

  //Set the color
  setColor(gameColors[colorIndex]);
}

//This is the loop for the board_idle state
void boardLoop(){
  if(isGameBoard || isSpinnerRing || isJoint){
    colorIndex = (myID%3);
  }

  if(isPlayer){
    colorIndex = myID;
  }

  //Set the color
  if(isJoint){
    flashColor(gameColors[colorIndex]);
  }
  else{
    setColor(gameColors[colorIndex]);
  }

  if(buttonDoubleClicked() && isBrain && !isSpinning){
    spinSpinner();
  }
  
  //Spin the Spinner
  if(isSpinning && isBrain)
    spinSpinner();
  if(isSpinnerRing){
    FOREACH_FACE(f){
      if(getLastValueReceivedOnFace(f) == SPIN_TARGET){
        setColor(gameColors[colorIndex]);
        setColorOnFace(gameColors[6],f);
        break;
      }
    }
  }

  //Check for game launch
  if(isBrain && launchingGame && roundTimer.isExpired()){
    launchingGame = false;
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

  if(!isBrain){
    FOREACH_FACE(f){
      if(!isValueReceivedOnFaceExpired(f)){
        switch(getLastValueReceivedOnFace(f)){
          case LAUNCH_RLGL:
            state = RLGL_SETUP;
            break;
          case LAUNCH_CHEERS:
            state = CHEERS_SETUP;
            break;
          case LAUNCH_CHAIRS:
            state = CHAIRS_SETUP;
            break;
        }
        if(state != BOARD_IDLE)
          break;
      }
    }
  }
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
  isPlayer = faceCount <= 1;
  isBrain = faceCount == 6;
}

//For the board - Check if we have a state change signal being sent at us, or if we recieve the state change handshake 
void checkBoardStateChange(){
  FOREACH_FACE(f){
    if(!isValueReceivedOnFaceExpired(f)){
      //Is the value being recieved A "Build board" signal? if so, we are a board piece
      if(getLastValueReceivedOnFace(f) >= BUILD_BOARD){
        state = BOARD_IDLE;
        myID = getLastValueReceivedOnFace(f) - BUILD_BOARD;
        setValueSentOnFace(BUILD_BOARD+((myID+1)%3),oppositeFace(f));
        isGameBoard = true;
      }
      else if(getLastValueReceivedOnFace(f) >= SET_ID){ //If we get the "set_id" signal in the decimal column, that means we're just going to send the signal down, unless we're a player
        state = BOARD_IDLE;
        myID = getLastValueReceivedOnFace(f) - SET_ID;
        if(!isPlayer){
          setValueSentOnFace(getLastValueReceivedOnFace(f),oppositeFace(f));
          if(getLastValueReceivedOnFace(oppositeFace(f)) == PLAYER || isValueReceivedOnFaceExpired(oppositeFace(f))){ //Is our other neighbor a player?
            isJoint = true;
            //Pass it down
            setValueSentOnFace(BUILD_BOARD+((myID+1)%3),nextFaceCounterClockwise(f));
          }
          else{
            isSpoke = true;
          }
        }
        break;
      }
      else if(getLastValueReceivedOnFace(f) >= SET_SPINNER){ //This means this blink is receiving a signal from the brain, so it's the inner ring
        state = BOARD_IDLE;
        isSpinnerRing = true;
        myID = getLastValueReceivedOnFace(f) - SET_SPINNER;
        setValueSentOnFace(SET_ID+myID,oppositeFace(f));
        break;
      }
    }
  }
}

//Spin the spinner!
void spinSpinner(){
  if(!isSpinning){ //Are we already spinning?
    isSpinning = true;
    colorIndex = 8;
    spinCurrent = spins[spinIndex%COUNT_OF(spins)]; 
    roundTimer.set(SPIN_TIMER_START);
    currentTimeAmount = SPIN_TIMER_START;
  }
  else{ //The spinner is already spinning
    if(roundTimer.isExpired()){ //This interval is complete
      setValueSentOnFace(0, spinCurrent);
      if(currentTimeAmount < SPIN_TIMER_MAX){ //Is the spinner interval less than our max interval?
        currentTimeAmount = (currentTimeAmount*11)/10;
        roundTimer.set(currentTimeAmount);        
        spinCurrent = nextFaceClockwise(spinCurrent);
        setValueSentOnFace(SPIN_TARGET, spinCurrent);  
      }
      else{ //We are done spinning
        isSpinning = false;
        colorIndex = spinCurrent % 3;
        spinIndex++;
        roundTimer.set(LAUNCH_NEW_GAME);
        launchingGame = true;
      }
    }
    else{ //The interval is ongoing
      setColorOnFace(gameColors[6], spinCurrent);
    }
  }
}

///
//RLGL
///


///
//CHEERS
///
byte scores[6];
vector highestScore;

void cheersReset(){
  bumpCount = 0;
  allowBump = false;
  lastBumped = 0;
  roundTimerStarted = false;  
  launchingGame = false;
  cheersPlayerHue = 0;
  receivedCount = 0;
  for(byte i = 0; i < 6; i++){
    receivedPlayers[i] = 6;
    scores[i] = 0;
  }
  highestScore.a = 0;
  highestScore.b = 0;
}

void cheersSetupLoop(){
  //Reset vars
  cheersReset();
  //Change State Input
  if(buttonDoubleClicked() && isBrain){
    state = CHEERS_GAME;
    setValueSentOnAllFaces(START_GAME);
  }

  if(!isBrain)
    FOREACH_FACE(f){
      if(!isValueReceivedOnFaceExpired(f)){
        if(getLastValueReceivedOnFace(f) == START_GAME){
          state = CHEERS_GAME;
          setValueSentOnAllFaces(START_GAME);
          break;
        }
    }
  }
  
  if(isBrain || isSpinnerRing)
    setColor(CYAN);
  else if(isPlayer)
    setColor(GREEN);
  else
    setColor(WHITE);
}

void cheersGameLoop(){
  //Timer
  if(!roundTimerStarted){ //Timer needs to be set
    roundTimer.set(ROUND_LENGTH_CHEERS);
    roundTimerStarted = true;
  }
  
  //PLAYER - Allow bumps with other players
  if(isPlayer){ 
    setValueSentOnAllFaces(SET_ID + myID);
    if(!roundTimer.isExpired()){
      if(isAlone()){ //Check number of faces to see if we're alone. If we are, we allow bump
        allowBump = true;
        flashColor(makeColorHSB(cheersPlayerHue, 255, 255));
      }
      else if(allowBump && !roundTimer.isExpired()){ //ID check for when we're not alone
        allowBump = false;
        FOREACH_FACE(f){
          if(!isValueReceivedOnFaceExpired(f)){
            if(getLastValueReceivedOnFace(f) != lastBumped && getLastValueReceivedOnFace(f) >= SET_ID){
              bumpCount++;
              if(bumpCount == 30)
                bumpCount = 31;
              if(bumpCount == 19)
                bumpCount = 20;
              lastBumped = getLastValueReceivedOnFace(f);
              changeColorAfterCheers();
              break;
            }
          }
        }
      }
      else{ //Not alone but also not doing an id Check
        setColor(makeColorHSB(cheersPlayerHue, 255, 255));
      }
    }
    else{ //Time's up!
      setValueSentOnAllFaces(bumpCount);
      if(isAlone())
        setColor(makeColorHSB(cheersPlayerHue, 255, 255));
    }
  }
  
  //SPINNER AND RING - Display timer
  if(isBrain || isSpinnerRing){
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
   if(isBrain){
    if(roundTimerStarted && roundTimer.isExpired()){
      if(buttonDoubleClicked()){ //Reset
        state = BOARD_IDLE;
        setValueSentOnAllFaces(BUILD_BOARD);
      }
      byte receivedCount = 0;
      FOREACH_FACE(f){ //Get Scores
        scores[f] = getLastValueReceivedOnFace(f);
        if(scores[f] != START_GAME){
          receivedCount++;
          if(scores[f] > highestScore.a){
            highestScore.a = scores[f];
            highestScore.b = f;
          }
        }
      }
      if(receivedCount == 5){
        setValueSentOnFace(WIN, highestScore.b);
        FOREACH_FACE(f){
          if(scores[f] == 0){
            setValueSentOnFace(LOSE, f); 
          }
          else if(f != highestScore.b){
            setValueSentOnFace(NONE, f);
          }
        }
      }
    }
   }

  //Timer expired, let's send signals and resolve this game
  if(!isBrain && roundTimerStarted && roundTimer.isExpired()){
    FOREACH_FACE(f){
      if(!isValueReceivedOnFaceExpired(f)){
        if(!isPlayer)
          setValueSentOnFace(getLastValueReceivedOnFace(f), oppositeFace(f));
        switch(getLastValueReceivedOnFace(f)){
          case WIN:
            setColor(GREEN);
            break;
          case LOSE:
            setColor(RED);
            break;
          case NONE:
            setColor(dim(YELLOW, 50));
            break;
          case BUILD_BOARD:
            state = BOARD_IDLE;
            setValueSentOnAllFaces(BUILD_BOARD);
            break;
          default:
            break;
        }
        if(state == BOARD_IDLE)
          break;
      }
    }
  }
}

const byte HUE_INCREASE_AMT = 10;
void changeColorAfterCheers(){
  cheersPlayerHue += HUE_INCREASE_AMT;
}

///
//Musical Chairs
///

vector colorPairs[3];
byte usedColors[6];
bool generateColors;

void chairsReset(){
  for(byte j = 0; j < 3; j++){
    colorPairs[j] = {6,6};
  }
  generateColors = true;
  for(byte k = 0; k < 6; k++){
    usedColors[k] = k;
  }
}

void chairsSetupLoop(){
  //Reset vars
  chairsReset();
  //Change State Input
  if(buttonDoubleClicked() && isBrain){
    state = CHAIRS_GAME;
    setValueSentOnAllFaces(START_GAME);
  }

  if(!isBrain)
    FOREACH_FACE(f){
      if(!isValueReceivedOnFaceExpired(f)){
        if(getLastValueReceivedOnFace(f) == START_GAME){
          state = CHAIRS_GAME;
          setValueSentOnAllFaces(START_GAME);
          break;
        }
    }
  }
  
  if(isBrain || isSpinnerRing)
    setColor(MAGENTA);
  else if(isPlayer)
    setColor(YELLOW);
  else
    setColor(WHITE);
}



void chairsGameLoop(){
  //Generate 3 color pairs
  if(isBrain){
    if(generateColors){
      //Shuffle colors
      byte r;
      byte temp;
      for(byte j = 0; j < 6; j++){
        r = random(5);
        temp = usedColors[j];
        usedColors[j] = usedColors[r];
        usedColors[r] = temp;
      }
      
      //Assign
      for(byte k = 0; k < 6; k++){    
        if(k < 3)
          colorPairs[k].a = usedColors[k];
        else
          colorPairs[k%3].b = usedColors[k];
        setColorOnFace(gameColors[usedColors[k]], k);
      }
      FOREACH_FACE(f){
        if(f < 3)
          setValueSentOnFace(colorPairs[f].a * 10 + colorPairs[f].b, f);
        else
          setValueSentOnFace(colorPairs[f%3].b * 10 + colorPairs[f%3].a, f);
      }
      generateColors = false;
    }
    else if(buttonPressed()){
      generateColors = true;
    }
  }
  //Get our colors
  if(!isBrain && generateColors){
    
  }
}


//Utilities

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

// Find opposite face
byte oppositeFace( byte f ) {
  return normalizeFace( f + (FACE_COUNT/2) );
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
