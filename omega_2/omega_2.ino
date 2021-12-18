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
#define RED_LIGHT 41
#define GREEN_LIGHT 42
#define WIN 60
#define LOSE 61
#define NONE 62
#define HARD_RESET 63

const unsigned int   SPIN_TIMER_MAX = 500;
const byte           SPIN_TIMER_START = 100;
const unsigned int   LAUNCH_NEW_GAME = 500;
const unsigned int   ROUND_LENGTH_CHEERS = 30000;
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
const byte spins[] = {0,1,2,1,0,2};
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
byte score; //ALSO USED FOR RLGL
//byte cheersPlayerHue; //Color hue
//Brain
byte receivedCount;
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
      setValueSentOnAllFaces(LAUNCH_RLGL);
      rlglSetupLoop();
      break;
    case RLGL_GAME:
      rlglGameLoop();
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
  if(isGameBoard || isJoint){
    colorIndex = (myID%3);
  }

  if(isSpoke || isSpinnerRing){
    colorIndex = 6;
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
        setColor(gameColors[myID%3]);
        //setColorOnFace(gameColors[6],f);
        setValueSentOnFace(SPIN_TARGET,normalizeFace(f+3));
        break;
      }
      if(getLastValueReceivedOnFace(f) == NONE){
        setValueSentOnFace(NONE,normalizeFace(f+3));
        break;
      }
    }
  }
  if(isSpoke){
    FOREACH_FACE(f){
      if(getLastValueReceivedOnFace(f) == SPIN_TARGET){
        setColor(gameColors[myID%3]);
        setValueSentOnFace(SPIN_TARGET,normalizeFace(f+3));
        break;
      }
      if(getLastValueReceivedOnFace(f) == NONE){
        setValueSentOnFace(NONE,normalizeFace(f+3));
        break;
      }
    }
  }

  //Check for game launch
  if(isBrain && launchingGame && roundTimer.isExpired()){
    launchingGame = false;
    switch((spinCurrent % 3)){
      case 0:
        state = RLGL_SETUP;
        break;
      case 1:
        state = CHEERS_SETUP;
        break;
      case 2:
        state = CHAIRS_SETUP;
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
    if(!isValueReceivedOnFaceExpired(f) && didValueOnFaceChange(f)){
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
      setValueSentOnFace(NONE, spinCurrent);
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
      setColorOnFace(gameColors[spinCurrent%3], spinCurrent);
    }
  }
}

///
//RLGL
///

//GAME STATES
enum gameStates {
  READY, //waiting for game to start, set teams
  REDLIGHT, //Middle blink, red state. Any neighbor will lose points if clicked
  GREENLIGHT, //Middle blink, green state. Any neighbor will gain points if clicked
  GAIN_POINTS,  //player piece, able to gain points when greenlight
  LOSE_POINTS, //player piece, loses half of total points when redlight
  WINNER, //player piece, displays win
  LOSER //player piece, displays loss
};
byte mode; //start at this mode
byte receivingFace;

bool isRippling; //flag to check if the middle blink is about to change. used for a display function that ripples .3 seconds before switching to red. used as a display warning
bool isGreenLight;  //check if light is green or red
bool roundOver; //checks if the light changes in order to set timers


//MIDDLE BLINK / REDLIGHT GREENLIGHT INTERVALS

#define RED_INTERVAL_MAX 4000
#define RED_INTERVAL_MIN 2000

#define GREEN_INTERVAL_MAX 4000
#define GREEN_INTERVAL_MIN 2000

#define RIPPLING_INTERVAL 300
#define CLICKTHRESHOLD 10

//Timers
Timer lightTimer; //Timer for how long the middle blink is red or green. Is randomized between a min and max range.
Timer ripplingTimer; //Timer for how long middle blink ripples before changing to red


//Winning and Losing Animations
const byte ROTATION_MS_PER_STEP = 50; //Winning blink displays a rotating pip, all others turn off.

void rlglReset(){
  mode = READY;
  isGreenLight = false;  //check if light is green or red
  roundOver = false; //checks if the light changes in order to set timers
  score = 0;
  receivingFace = 6;
}

void rlglSetupLoop() {

  //Reset vars
  rlglReset();
  
  //Change State Input
  if(buttonDoubleClicked() && isBrain){
    state = RLGL_GAME;
    setValueSentOnAllFaces(START_GAME);
  }

  if(!isBrain)
    FOREACH_FACE(f){
      if(!isValueReceivedOnFaceExpired(f)){
        if(getLastValueReceivedOnFace(f) == START_GAME){
          state = RLGL_GAME;
          setValueSentOnAllFaces(START_GAME);
          break;
        }
    }
  }

  if(isPlayer)
    setValueSentOnAllFaces(0);

  if(isBrain || isSpinnerRing)
    setColor(RED);
  else if(isPlayer)
    setColor(CYAN);
  else
    setColor(WHITE);
}


void rlglGameLoop() {
  
  switch ( mode )  {
    
    case READY:
      readyLoop();
      break;
    case REDLIGHT:
      setValueSentOnAllFaces(RED_LIGHT);
      redLightLoop();
      break;

    case GREENLIGHT:
      setValueSentOnAllFaces(GREEN_LIGHT);
      greenLightLoop();
      break;

    case LOSE_POINTS:
      losePointsLoop();
      break;

    case GAIN_POINTS:
      gainPointsLoop();
      break;

    case WINNER:
      setValueSentOnAllFaces(WIN);
      winnerLoop();
      break;

    case LOSER:
      setValueSentOnAllFaces(LOSE);
      loserLoop();
      break;
  }
}

void readyLoop() {
  if (buttonDoubleClicked() && isBrain) { //long press middle blink to set it to the middle light and START GAME
    mode = REDLIGHT; //start at red light
    roundOver = true;
    isGreenLight = false;
  }
  //checks neighbors for REDLIGHT because the game starts on it. will change neighbors to LOSE_POINTS mode.
  if(isPlayer){
    FOREACH_FACE( f ) {
      if ( !isValueReceivedOnFaceExpired( f ) ) {
        byte neighbor = getLastValueReceivedOnFace( f );
        bool didNeighborJustChange = didValueOnFaceChange( f );
        if (neighbor == RED_LIGHT && didNeighborJustChange) { //if there is a red light neighbor
          mode = LOSE_POINTS; //change self to losing point mode
        }
      }
    }
  }

  if(isSpoke || isJoint || isSpinnerRing){
    scoreDisplay();
  }
  
  if(!isPlayer && !isBrain){
    FOREACH_FACE( f ) {
      if ( !isValueReceivedOnFaceExpired( f ) ) {
          if(getLastValueReceivedOnFace(f) == RED_LIGHT){
            receivingFace = normalizeFace(f+3);
            setValueSentOnFace(getLastValueReceivedOnFace(f), (f+3)%6);
          }
          if(getLastValueReceivedOnFace(f) == GREEN_LIGHT)
          {
            setValueSentOnFace(getLastValueReceivedOnFace(f), (f+3)%6);
          }
          if(getLastValueReceivedOnFace(f) == WIN){
            mode = LOSER;
          }
          if(getLastValueReceivedOnFace(f) == LOSE){
            setValueSentOnAllFaces(getLastValueReceivedOnFace(f));
            mode = LOSER;
          }
      }
    }
  }

  //Draw
  if(isPlayer)
    setColor(gameColors[colorIndex]); //Set Team
  else if(isBrain)
    setColor(CYAN);
}

void redLightLoop() { //loop for middle blink when it is red
  if (roundOver == true && isGreenLight == false) { //roundOver checks if the light has changed
    lightTimer.set(RED_INTERVAL_MIN + random(RED_INTERVAL_MAX));
    roundOver = false;
  }
  setColor(RED);
  if (lightTimer.isExpired()) { //when red light timer is over, change to green light
    roundOver = true;
    isGreenLight = true;
    mode = GREENLIGHT;
  }
  listenForWinner();
}

void greenLightLoop() { //loop for middle blink when it is green
  if (roundOver == true && isGreenLight == true) {
    lightTimer.set(GREEN_INTERVAL_MIN + random(GREEN_INTERVAL_MAX)); //set random timer for green light
    roundOver = false;
  }
  setColor(GREEN);
  if (lightTimer.getRemaining() > 0 && lightTimer.getRemaining() < 301) { //in the last .3 seconds, ripple to send a visual warning
    ripplingTimer.set(RIPPLING_INTERVAL);
  }
  if (!ripplingTimer.isExpired()) {
    FOREACH_FACE(f) {
      setColorOnFace(makeColorHSB(70, 255, random(50) + 205), f); //70 is GREEN HUE, ripple while the timer is not expired
    }
  }
  if (lightTimer.isExpired()) { //when green light timer expires, change to red right
    roundOver = true;
    isGreenLight = false;
    mode = REDLIGHT;
  }
  listenForWinner();
}

void losePointsLoop() { //player piece (outer ring) loop when the middle blink/light is red

//checks for all sorts of button clicks to reset the player score is any of them are performed

  if (buttonPressed())
  {
    score = 0; //add multi-clicks to array of scores that align with team colours
    setValueSentOnAllFaces(score);
  }

  FOREACH_FACE( f ) {
    if ( !isValueReceivedOnFaceExpired( f ) ) {
      byte neighbor = getLastValueReceivedOnFace( f ); //a neighbor!
      bool didNeighborJustChange = didValueOnFaceChange( f ); //did the neighbor value recently change?
      if (neighbor == GREEN_LIGHT && didNeighborJustChange) { //if there is a green light neighbor and it recently changed
        mode = GAIN_POINTS; //change self to gain points mode
      }
    }
  }

  scoreDisplay(); //display score blips on player blink
  listenForWinner(); // listen for a winning blink
}



void gainPointsLoop() { //player piece loop for when middle blink/light is green

  FOREACH_FACE( f ) {
    if ( !isValueReceivedOnFaceExpired( f ) ) {
      byte neighbor = getLastValueReceivedOnFace( f ); //a neighbor!
      bool didNeighborJustChange = didValueOnFaceChange( f ); //did neighbor recently change?
      if (neighbor == RED_LIGHT && didNeighborJustChange) { //if there is a red light neighbor and it recently changed
        mode = LOSE_POINTS; //change self to lose points mode
      }
    }
  }


  if (buttonPressed()) { //add score on spam click or click
    score += 1;
    setValueSentOnAllFaces(score);
  }

  scoreDisplay(); //display score
  listenForWinner();  //listen for a winning blink
}


void scoreDisplay() {
  if(isSpoke || isJoint || isSpinnerRing){
    FOREACH_FACE(f){
      if(f == receivingFace){
        setColor(dim(gameColors[myID],min(255, getLastValueReceivedOnFace(f) * 25))); 
        if(getLastValueReceivedOnFace(f)-CLICKTHRESHOLD > 0){
          setValueSentOnFace(getLastValueReceivedOnFace(f) - CLICKTHRESHOLD, oppositeFace(f));
        }
        else{
          setValueSentOnFace(0, oppositeFace(f));
        }
      }
    }
  }
  
  if(isPlayer){
    if (score >= CLICKTHRESHOLD * 4) {
      //setColorOnFace(WHITE, 5);
      //whoever gets to 60 wins!
     mode = WINNER;
    }
    setColor(gameColors[colorIndex]);
  }
}

void winnerLoop() {
    byte rotationFace = (millis() / ROTATION_MS_PER_STEP) % FACE_COUNT; // winning animation, rotating face
    setColor(gameColors[colorIndex]); //set background to team colour
    setColorOnFace( WHITE , rotationFace ); //set the rotating face colour

    if(!isBrain){
      FOREACH_FACE(f){
        if(!isValueReceivedOnFaceExpired(f)){
          if(getLastValueReceivedOnFace(f) == BUILD_BOARD){
            state = BOARD_IDLE;
            setValueSentOnAllFaces(BUILD_BOARD);
            break;
          }
        }
      }
    }
}

void loserLoop() { //dim then turn off all other blinks
    setColor(RED);
    if(isBrain && buttonDoubleClicked()){
        state = BOARD_IDLE;
        setValueSentOnAllFaces(BUILD_BOARD);
    }

    if(!isBrain){
      FOREACH_FACE(f){
        if(!isValueReceivedOnFaceExpired(f)){
          if(getLastValueReceivedOnFace(f) == BUILD_BOARD){
            state = BOARD_IDLE;
            setValueSentOnAllFaces(BUILD_BOARD);
            break;
          }
        }
      }
    }
}

void listenForWinner() { //outer blinks will listen for a winning blink throughout the game
  FOREACH_FACE( f ) {
    if ( !isValueReceivedOnFaceExpired( f ) ) {
      byte neighbor = getLastValueReceivedOnFace( f ); //found neighbor!
      if (neighbor == WIN) { //if there is a winner neighbor
        if(f == receivingFace){
          mode = WINNER;
          break; 
        }
        else{
          mode = LOSER; //change self to loser
        }
      }
      if (neighbor == LOSE) { //if there is a loser neighbor
        mode = LOSER; //change self to loser
      }
    }
  }
}

///
//CHEERS
///
byte scores[6];
byte highestScore;

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
    setColor(BLUE);
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
    if(!roundTimer.isExpired()){
      if(isAlone()){ //Check number of faces to see if we're alone. If we are, we allow bump
        allowBump = true;
        flashColor(gameColors[colorIndex]);
        setValueSentOnAllFaces(SET_ID + myID);
      }
      else if(allowBump){ //ID check for when we're not alone
        allowBump = false;
        FOREACH_FACE(f){
          if(!isValueReceivedOnFaceExpired(f)){
            if(getLastValueReceivedOnFace(f) != lastBumped && getLastValueReceivedOnFace(f) >= SET_ID){
              score++;
              if(score == BUILD_BOARD){
                score++;
              }
              if(score == START_GAME){
                score++;
              }
              lastBumped = getLastValueReceivedOnFace(f);
              //changeColorAfterCheers();
              break;
            }
          }
        }
      }
      else{ //Not alone but also not doing an id Check
        setValueSentOnAllFaces(score);
        setColor(gameColors[colorIndex]);
      }
    }
    else{ //Time's up!
      setValueSentOnAllFaces(score);
      if(isAlone()){
        setColor(gameColors[colorIndex]);
      }
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
      byte receivedCount = 0;
      FOREACH_FACE(f){ //Get Scores
        if(getLastValueReceivedOnFace(f) != START_GAME){
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
   }

  //Timer expired, let's send signals and resolve this game
  if(!isBrain && roundTimerStarted && roundTimer.isExpired()){
    FOREACH_FACE(f){
      if(!isValueReceivedOnFaceExpired(f)){
        if(getLastValueReceivedOnFace(f) == BUILD_BOARD){
          state = BOARD_IDLE;
          setValueSentOnAllFaces(BUILD_BOARD);
          break;
        }
        if(isJoint || isSpoke || isSpinnerRing){
          setValueSentOnFace(getLastValueReceivedOnFace(f), oppositeFace(f));
        }
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
          default:
            break;
        }          
      }
    }
  }
}

///
//Musical Chairs
///

bool generateColors;
bool gameOver;

void chairsReset(){
  generateColors = true;
  gameOver = false;
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
    setColor(GREEN);
  else if(isPlayer)
    setColor(YELLOW);
  else
    setColor(WHITE);
}



void chairsGameLoop(){
  if(gameOver || isGameBoard){
    if(isBrain && buttonDoubleClicked()){
      state = BOARD_IDLE;
      setValueSentOnAllFaces(BUILD_BOARD);
    }
    if(!isBrain){
      FOREACH_FACE(f){
        if(!isValueReceivedOnFaceExpired(f)){
          if(getLastValueReceivedOnFace(f) == BUILD_BOARD){
            state = BOARD_IDLE;
            setValueSentOnAllFaces(BUILD_BOARD);
            break;
          }
        }
      }
    }
    return;
  }
  //Generate 3 color pairs
  if(isBrain){
    if(generateColors){ 
      //Assign
      FOREACH_FACE(f){
        setColorOnFace(gameColors[normalizeFace(f+3)], f);
        setValueSentOnFace(normalizeFace(f+3), f);
      }
      generateColors = false;
    }
    else{
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
  }
  //Get our colors
  if((isSpoke || isJoint || isSpinnerRing) && generateColors){
    FOREACH_FACE(f){
      if(!isValueReceivedOnFaceExpired(f)){
        if(getLastValueReceivedOnFace(f) != START_GAME && getLastValueReceivedOnFace(f) != LAUNCH_CHAIRS && getLastValueReceivedOnFace(f) != BUILD_BOARD){
          if(!isJoint){
            setValueSentOnFace(getLastValueReceivedOnFace(f),oppositeFace(f));
          }
          colorIndex = getLastValueReceivedOnFace(f);
          generateColors = false;
          break;
        }
      }
    }
  }

  //Players need to attach to their matching color
  if(isPlayer){
    if(isAlone()){
      setValueSentOnAllFaces(SET_ID + colorIndex);
    }
  }
  
  if(!generateColors && (isSpoke || isJoint || isSpinnerRing)){
    FOREACH_FACE(f){
      if(!isValueReceivedOnFaceExpired(f)){
        if(getLastValueReceivedOnFace(f) == WIN || getLastValueReceivedOnFace(f) == LOSE){
          setValueSentOnFace(getLastValueReceivedOnFace(f), oppositeFace(f));
        }
      }
    }
  }

  if(isJoint && !generateColors){
    FOREACH_FACE(f){
      if(!isValueReceivedOnFaceExpired(f)){
        if(getLastValueReceivedOnFace(f) >= SET_ID){
          if(getLastValueReceivedOnFace(f) - SET_ID == colorIndex){
              setValueSentOnAllFaces(WIN);
              setColor(GREEN);
              gameOver = true;
              break;
          }
        }
      }
    }
  }
  
  if((isPlayer || isSpoke || isSpinnerRing || isJoint)){
    winLoseCheck();
    if(!gameOver){
      setColor(gameColors[colorIndex]);
    }   
  }
}

void winLoseCheck(){
  FOREACH_FACE(f){
      if(!isValueReceivedOnFaceExpired(f)){
        if(getLastValueReceivedOnFace(f) == LOSE && !gameOver){
          setColor(RED);
          gameOver = true;
        }
        else if(getLastValueReceivedOnFace(f) == WIN && !gameOver){
          setColor(GREEN);
          gameOver = true;
        }
      }
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
