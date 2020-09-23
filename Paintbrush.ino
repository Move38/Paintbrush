/*
    Paintbrush
    by Carol Mertz and Mary McKenzie
    for Blinks by Move38

    Rules: https://move38.com/games

    --------------------
    Blinks by Move38
    Brought to life via Kickstarter 2020

    @madewithblinks
    www.move38.com
    --------------------
*/

enum wipeStates {INERT, WIPING, RESOLVE};
byte wipeState = INERT;

byte colorHues[] = {0, 40, 60, 100, 220};

byte faceColors[6] = {0, 0, 0, 0, 0, 0};//holds current state
byte saveStates[6] = {0, 0, 0, 0, 0, 0};//holds previous state
#define SAVE_STATE_DELAY 300
Timer saveStateTimer;

bool isBrush = false;

Timer brushCycle;
#define BRUSH_CYCLE_DURATION 100
byte brushFace = 0;

bool wakeFrame = false;

void setup() {
  randomize();
}

void loop() {

  // discard the button press from waking up
  wakeFrame = false;
  if (hasWoken()) {
    wakeFrame = true;
  }

  switch (wipeState) {
    case INERT:
      inertLoop();
      break;
    case WIPING:
      wipingLoop();
      break;
    case RESOLVE:
      resolveLoop();
      break;
  }

  //send data
  FOREACH_FACE(f) {
    byte sendData = (isBrush << 5) | (wipeState << 3) | (faceColors[f]);
    setValueSentOnFace(sendData, f);
  }

  //do display
  if (isBrush) {
    brushDisplay();
  } else {
    canvasDisplay();
  }
}

void inertLoop() {
  //do paint logic
  if (!isBrush) {

    //look for neighboring paint brushes
    // or neighboring paint
    // only paint brushes can paint over painted faces,
    // but paint can spread from painted canvas to blank canvas
    FOREACH_FACE(f) {
      if (!isValueReceivedOnFaceExpired(f)) {//neighbor!
        byte neighborData = getLastValueReceivedOnFace(f);
        if (getBrush(neighborData) == 1) { //this neighbor is a brush
          byte neighborColor = getColor(neighborData);
          if (faceColors[f] != neighborColor) {
            saveState();
            faceColors[f] = neighborColor;
          }
        }
        else {
          // not a brush, but could paint us
          if (faceColors[f] == 0) {
            // this is blank canvas, take on our neighbors color
            byte neighborColor = getColor(neighborData);
            if (neighborColor != faceColors[f]) {
              saveState();
              faceColors[f] = neighborColor;
            }
          }
        }
      }
    }

    //now that I've maybe changed color, do I become a brush?
    if (faceColors[0] > 0) { //face 0 has a real color
      byte brushCheck = faceColors[0];//save that color as the reference
      byte brushCheckCount = 0;

      FOREACH_FACE(f) {//run through the faces and check for that color
        if (faceColors[f] == brushCheck) {//is this face the correct color
          brushCheckCount++;
        }
      }

      if (brushCheckCount == 6) {//hey, we got 6 hits! become brush!
        isBrush = true;
      }
    }
  }

  //listen for button presses
  if (buttonSingleClicked() && wakeFrame == false) {//create or recolor brushes
    if (isBrush) {//change to next brush color
      byte nextColor = (faceColors[0] % 4) + 1;//determine the next color
      FOREACH_FACE(f) { //paint all faces that color
        faceColors[f] = nextColor;
      }
    } else {
      if (isBlank()) {//will only become a brush if blank
        isBrush = true;
        byte randomColor = random(3) + 1;//just choose a random color
        FOREACH_FACE(f) { //paint all faces that color
          faceColors[f] = randomColor;
        }
      }
    }
  }

  if (buttonLongPressed()) {//reset the blink to a blank canvas
    isBrush = false;
    FOREACH_FACE(f) {
      faceColors[f] = 0;
    }
  }

  if (buttonDoubleClicked()) {//revert to save state
    if (!isBrush) {//only canvasses can do this
      FOREACH_FACE(f) {
        faceColors[f] = saveStates[f];
      }
    }
  }

  if (buttonMultiClicked()) {//if long-pressed, begin field wiping
    if (buttonClickCount() == 3) {
      wipeState = WIPING;
      FOREACH_FACE(f) {
        faceColors[f] = 5; //this is the special "wiping" color
      }
    }
  }

  FOREACH_FACE(f) {//check around for anyone in WIPING
    if (!isValueReceivedOnFaceExpired(f)) {//neighbor!
      if (getWipeState(getLastValueReceivedOnFace(f)) == WIPING && getColor(getLastValueReceivedOnFace(f)) == 5) {//we only accept people in the right state AND the special color handshake
        wipeState = WIPING;
        FOREACH_FACE(f) {
          faceColors[f] = 5; //this is the special "wiping" color
        }
      }
    }
  }
}

void saveState() {
  //so when we are asked to save, we should make sure we haven't been asked recently
  if (saveStateTimer.isExpired()) {//ok, so the timer is not running, we can save this state
    FOREACH_FACE(f) {//put all the colors into the save state
      saveStates[f] = faceColors[f];
    }

    //then start the timer so we ignore input for a little bit
    saveStateTimer.set(SAVE_STATE_DELAY);
  }
}

void wipingLoop() {
  bool canResolve = true;//we default to this, but revert it in the loop below if we need to

  FOREACH_FACE(f) {//check around for anyone still INERT
    if (!isValueReceivedOnFaceExpired(f)) {//neighbor!
      if (getWipeState(getLastValueReceivedOnFace(f)) == INERT) {
        canResolve = false;
      }
    }
  }

  if (canResolve) {
    wipeState = RESOLVE;
    isBrush = false;
    FOREACH_FACE(f) {
      faceColors[f] = 0;
    }
  }
}

void resolveLoop() {
  bool canInert = true;//we default to this, but revert it in the loop below if we need to

  FOREACH_FACE(f) {//check around for anyone still WIPING
    if (!isValueReceivedOnFaceExpired(f)) {//neighbor!
      if (getWipeState(getLastValueReceivedOnFace(f)) == WIPING) {
        canInert = false;
      }
    }
  }

  if (canInert) {
    wipeState = INERT;
  }
}

void canvasDisplay() {
  FOREACH_FACE(f) {
    if (faceColors[f] > 0) {//colored faces are at full brightness
      setColorOnFace(makeColorHSB(colorHues[faceColors[f]], 255, 255), f);
    } else {//blank faces are at 0 brightness
      setColorOnFace(makeColorHSB(0, 0, 40), f);
    }
  }

  //  //debug graphics for saveState
  //  if (!saveStateTimer.isExpired()) {
  //    byte timerProgress = map(saveStateTimer.getRemaining(), 0, SAVE_STATE_DELAY, 0, 255);
  //    setColor(dim(ORANGE, timerProgress));
  //  }
}

void brushDisplay() {
  //  wipe the face with the color chasing it's tail
  //  slight hue offset to feel painterly
  if (brushCycle.isExpired()) {
    brushFace = (brushFace + 1) % 6;
    brushCycle.set(BRUSH_CYCLE_DURATION);
  }

  // Adjust the offset of the paint hue to make each of the hue shifts subtle and noticeable
  int brushColorOffset = 2;
  if (faceColors[0] == 1) { // YELLOW - cheat this one orange
    brushColorOffset = -2;
  }
  else if (faceColors[0] == 3) { // BLUE - compensate for large blue range
    brushColorOffset = 3;
  }

  FOREACH_FACE(f) {
    setColorOnFace(makeColorHSB((colorHues[faceColors[f]] + brushColorOffset * f) % 255, 255, 255), (brushFace + f) % 6);
  }
}

byte getBrush(byte data) {
  return (data >> 5);//only the first bit
}

byte getWipeState(byte data) {
  return ((data >> 3) & 3);//the second and third bit
}

byte getColor(byte data) {//the last 3 bits
  return (data & 7);
}

bool isBlank () {
  bool blankBool = true;

  FOREACH_FACE(f) {
    if (faceColors[f] > 0) {
      blankBool = false;
    }
  }

  return blankBool;
}
