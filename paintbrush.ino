byte colorHues[6] = {30, 60, 100, 140, 200, 220};

byte faceColors[6] = {6, 6, 6, 6, 6, 6};
bool isBrush = false;

Timer brushCycle;
#define BRUSH_CYCLE_DURATION 100
byte brushFace = 0;

void setup() {
  randomize();
}

void loop() {

  //do paint logic
  if (!isBrush) {

    //look for neighboring paint brushes
    FOREACH_FACE(f) {
      if (!isValueReceivedOnFaceExpired(f)) {//neighbor!
        byte neighborData = getLastValueReceivedOnFace(f);
        if (getBrush(neighborData) == 1) { //this neighbor is a brush
          byte neighborColor = getColor(neighborData);
          if (faceColors[f] != neighborColor) {
            faceColors[f] = neighborColor;
          }
        }
      }
    }

    //now that I've maybe changed color, do I become a brush?
    if (faceColors[0] < 6) { //face 0 has a real color
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
  if (buttonSingleClicked()) {//create or recolor brushes
    if (isBrush) {//change to next brush color
      byte nextColor = (faceColors[0] + 1) % 6;//determine the next color
      FOREACH_FACE(f) { //paint all faces that color
        faceColors[f] = nextColor;
      }
    } else {
      byte randomColor = random(5);//just choose a random color
      FOREACH_FACE(f) { //paint all faces that color
        faceColors[f] = randomColor;
      }
    }
  }

  if (buttonLongPressed()) {//reset the blink to a blank canvas
    isBrush = false;
    FOREACH_FACE(f) {
      faceColors[f] = 6;
    }
  }

  //send data
  FOREACH_FACE(f) {
    byte sendData = (isBrush << 5) + (faceColors[f]);
    setValueSentOnFace(sendData, f);
  }

  //do display
  if (isBrush) {
    brushDisplay();
  } else {
    canvasDisplay();
  }
}

void canvasDisplay() {
  FOREACH_FACE(f) {
    if (faceColors[f] < 6) {//colored faces are at full brightness
      setColorOnFace(makeColorHSB(colorHues[faceColors[f]], 255, 255), f);
    } else {//blank faces are at 0 brightness
      setColorOnFace(makeColorHSB(0, 0, 0), f);
    }
  }
}

void brushDisplay() {
  //  Option 1: simple pulse for brush
  //  setColor(makeColorHSB(colorHues[faceColors[0]], 255, 63 + (3 * sin8_C(millis()/15))/4));

  //  wipe the face with the color chasing it's tail
  //  slight hue offset to feel painterly
  if(brushCycle.isExpired()) {
    brushFace = (brushFace + 1) % 6;
    brushCycle.set(BRUSH_CYCLE_DURATION);
  }
  FOREACH_FACE(f) {
    setColorOnFace(makeColorHSB((colorHues[faceColors[f]] + 2*f)%255, 255, 255), (brushFace + f) % 6); 
  }
}

byte getBrush(byte data) {
  return (data >> 5);
}

byte getColor(byte data) {
  return (data & 7);
}
