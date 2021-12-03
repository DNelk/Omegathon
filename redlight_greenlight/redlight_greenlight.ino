//RED LIGHT, GREEN LIGHT!
//FUTURE IDEA, Maybe: Co-op! all players must reach 60 points to win. if one person clicks on red, everyone loses their points.


//How to play:
//Each player (up to 6) claims a Blink on the outer ring. Double click to choose team colour. The middle Blink is unclaimed- it is the light.
//Long press the middle blink to begin

//The middle Blink will switch between 2 states at random- RED and GREEN.
//When the middle blink is Green, every player must spam click their blink as many times as they can. 1 click = 1 point.
//When the middle blink turns Red, every player must stop clicking, or they will lose all of their points.
//The winner reaches 60 points first. Ties are possible.

//TIP: the middle blink will flicker slightly- giving a very short warning before it turns red.
//TIP: There is no warning for when the middle blink turns from red to green, so stay alert!


//TEAM AND SCORES
byte teamHues[6] = {22, 49, 82, 99, 160, 200}; //team colours
byte teamScores[6] = {0, 0, 0, 0, 0, 0}; //scores, also uses teamIndex to line up with team hues
byte teamIndex = 0;
bool isPlayer = false;
bool isBrain = false;
byte faceCount;

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
byte mode = READY; //start at this mode


bool isRippling; //flag to check if the middle blink is about to change. used for a display function that ripples .3 seconds before switching to red. used as a display warning
bool isGreenLight = false;  //check if light is green or red
bool roundOver = false; //checks if the light changes in order to set timers


//MIDDLE BLINK / REDLIGHT GREENLIGHT INTERVALS

#define RED_INTERVAL_MAX 4000
#define RED_INTERVAL_MIN 2000

#define GREEN_INTERVAL_MAX 4000
#define GREEN_INTERVAL_MIN 2000

#define RIPPLING_INTERVAL 300
#define CLICKTHRESHOLD 20

//Timers
Timer lightTimer; //Timer for how long the middle blink is red or green. Is randomized between a min and max range.
Timer ripplingTimer; //Timer for how long middle blink ripples before changing to red


//Winning and Losing Animations
const byte ROTATION_MS_PER_STEP = 50; //Winning blink displays a rotating pip, all others turn off.


void setup() {
  mode = READY; //start at READY
  setColor(WHITE);
}


void loop() {
  
  switch ( mode )  {
    
    case READY:
      readyLoop();
      break;

    case REDLIGHT:
      setValueSentOnAllFaces(mode);
      redLightLoop();
      break;

    case GREENLIGHT:
      setValueSentOnAllFaces(mode);
      greenLightLoop();
      break;

    case LOSE_POINTS:
      losePointsLoop();
      break;

    case GAIN_POINTS:
      gainPointsLoop();
      break;

    case WINNER:
      setValueSentOnAllFaces(mode);
      winnerLoop();
      break;

    case LOSER:
      setValueSentOnAllFaces(mode);
      loserLoop();
      break;
  }

  buttonSingleClicked();
  buttonDoubleClicked();
}


Color getColorForTeam(byte t) { //returns Team Color
  return makeColorHSB(teamHues[t], 255, 255);
}

void readyLoop() {
  faceCount = 0;
  FOREACH_FACE(f){
    if(!isValueReceivedOnFaceExpired(f)){
      faceCount++;
    }
  }
  //Check Players and Spinner
  isPlayer = faceCount <= 1;
  isBrain = faceCount == 6;
  
  if (buttonPressed() && isPlayer) { //Change Teams
    teamIndex++;
    if (teamIndex > COUNT_OF(teamHues)) { //if reaching the end, reset to beginning of team hues array
      teamIndex = 0;
    }
  }

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
        if (neighbor == REDLIGHT && didNeighborJustChange) { //if there is a red light neighbor
          mode = LOSE_POINTS; //change self to losing point mode
        }
      }
    }
  }

  if(!isPlayer && !isBrain){
    FOREACH_FACE( f ) {
      if ( !isValueReceivedOnFaceExpired( f ) ) {
          setValueSentOnFace(getLastValueReceivedOnFace(f), (f+3)%6);
          if(getLastValueReceivedOnFace(f) == WINNER){
            mode = LOSER;
          }
          if(getLastValueReceivedOnFace(f) == LOSER){
            setValueSentOnAllFaces(getLastValueReceivedOnFace);
            mode = LOSER;
          }
      }
    }
  }

  //Draw
  if(isPlayer)
    setColor(getColorForTeam(teamIndex)); //Set Team
  else if(isBrain)
    setColor(CYAN);
  else
    setColor(WHITE);
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
}

void losePointsLoop() { //player piece (outer ring) loop when the middle blink/light is red

//checks for all sorts of button clicks to reset the player score is any of them are performed

  if (buttonSingleClicked())
  {
    teamScores[teamIndex] = 0; //add multi-clicks to array of scores that align with team colours
  }

  if (buttonDoubleClicked()) {
    teamScores[teamIndex] = 0;
  }


  if (buttonDown()) {
    teamScores[teamIndex] = 0;
  }

  if (buttonReleased()) {
    teamScores[teamIndex] = 0;
  }

  if (teamScores[teamIndex] < 0) { //if value drops below 0 just round it to 0, just in case
    teamScores[teamIndex] = 0;
  }

  FOREACH_FACE( f ) {
    if ( !isValueReceivedOnFaceExpired( f ) ) {
      byte neighbor = getLastValueReceivedOnFace( f ); //a neighbor!
      bool didNeighborJustChange = didValueOnFaceChange( f ); //did the neighbor value recently change?
      if (neighbor == GREENLIGHT && didNeighborJustChange) { //if there is a green light neighbor and it recently changed
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
      if (neighbor == REDLIGHT && didNeighborJustChange) { //if there is a red light neighbor and it recently changed
        mode = LOSE_POINTS; //change self to lose points mode
      }
    }
  }


  if (buttonReleased()) { //add score on spam click or click
    teamScores[teamIndex] += 1;
  }

  scoreDisplay(); //display score
  listenForWinner();  //listen for a winning blink
}


void scoreDisplay() {

  if (teamScores[teamIndex] > CLICKTHRESHOLD) { //if greater than 10, light up face 0
    setColorOnFace(WHITE, 0);
  }

  else {
    setColorOnFace(getColorForTeam(teamIndex), 0); //else, set face to off
  }
  if (teamScores[teamIndex] > CLICKTHRESHOLD * 2) {
    setColorOnFace(WHITE, 1);
  }
  else {
    setColorOnFace(getColorForTeam(teamIndex), 1);
  }

  if (teamScores[teamIndex] >  CLICKTHRESHOLD * 3) {
    setColorOnFace(WHITE, 2);
  }
  else {
    setColorOnFace(getColorForTeam(teamIndex), 2);
  }

  if (teamScores[teamIndex] >  CLICKTHRESHOLD * 4) {
    setColorOnFace(WHITE, 3);
  }
  else {
    setColorOnFace(getColorForTeam(teamIndex), 3);
  }

  if (teamScores[teamIndex] >  CLICKTHRESHOLD * 5) {
    setColorOnFace(WHITE, 4);
  }
  else {
    setColorOnFace(getColorForTeam(teamIndex), 4);
  }
  if (teamScores[teamIndex] == CLICKTHRESHOLD * 6) {
    setColorOnFace(WHITE, 5);
    //whoever gets to 60 wins!
    mode = WINNER;
    
  }
  else if (teamScores[teamIndex] <  CLICKTHRESHOLD * 6) { //light up face, but player hasn't won yet
    setColorOnFace(getColorForTeam(teamIndex), 5);
  }

}

void winnerLoop() {
    byte rotationFace = (millis() / ROTATION_MS_PER_STEP) % FACE_COUNT; // winning animation, rotating face
    setColor(getColorForTeam(teamIndex)); //set background to team colour
    setColorOnFace( WHITE , rotationFace ); //set the rotating face colour
}

void loserLoop() { //dim then turn off all other blinks
    setColor(dim(getColorForTeam(teamIndex), 20));
}

void listenForWinner() { //outer blinks will listen for a winning blink throughout the game
  FOREACH_FACE( f ) {
    if ( !isValueReceivedOnFaceExpired( f ) ) {
      byte neighbor = getLastValueReceivedOnFace( f ); //found neighbor!
      if (neighbor == WINNER) { //if there is a winner neighbor
        mode = LOSER; //change self to loser
      }
      if (neighbor == LOSER) { //if there is a loser neighbor
        mode = LOSER; //change self to loser
      }
    }
  }
}
