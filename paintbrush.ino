byte colorHues[7] = {0, 30, 60, 100, 140, 200, 220};

byte faceColors[6] = {0, 0, 0, 0, 0, 0};
bool isBrush = false;

void setup() {
  randomize();
}

void loop() {

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
            faceColors[f] = neighborColor;
          }
        }
        else {
          // not a brush, but could paint us
          if (faceColors[f] == 0) {
            // this is blank canvas, take on our neighbors color
            byte neighborColor = getColor(neighborData);
            faceColors[f] = neighborColor;
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
  if (buttonSingleClicked()) {//create or recolor brushes
    if (isBrush) {//change to next brush color
      byte nextColor = (faceColors[0] % 6) + 1;//determine the next color
      FOREACH_FACE(f) { //paint all faces that color
        faceColors[f] = nextColor;
      }
    } else {
      byte randomColor = random(5) + 1;//just choose a random color
      FOREACH_FACE(f) { //paint all faces that color
        faceColors[f] = randomColor;
      }
    }
  }

  if (buttonLongPressed()) {//reset the blink to a blank canvas
    isBrush = false;
    FOREACH_FACE(f) {
      faceColors[f] = 0;
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
    if (faceColors[f] > 0) {//colored faces are at medium brightness
      setColorOnFace(makeColorHSB(colorHues[faceColors[f]], 255, 150), f);
    } else {//blank faces are at a really minimal brightness
      setColorOnFace(makeColorHSB(0, 0, 40), f);
    }
  }
}

void brushDisplay() {//just show the color on full blast
  setColor(makeColorHSB(colorHues[faceColors[0]], 255, 255));
}

byte getBrush(byte data) {
  return (data >> 5);
}

byte getColor(byte data) {
  return (data & 7);
}
