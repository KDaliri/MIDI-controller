
/* Use arrays to manage lists of knobs/pots and pushbuttons.

   By Leif Oddson
   https://forum.pjrc.com/threads/45376

   Teensy 4.1

   Jan 21, 2024 - Version 1 - J. Alvarez
   Jan 31, 2024 - Version 2 - J. Alvarez - Added SSD1306 OLED Display Capabilities and ENTER keyboard stroke
   Feb 11, 2024 - Version 3 - J. Alvarez - Added Bank and Program Change functions
   Feb 29, 2024 - Version 4 - J. Alvarez - Added Mouse Feature
   Mar 01, 2024 - Version 5 - J. Alvarez - Offical Version 1 according to Hardware Block Diagram; Added Program Mode
*/

//************LIBRARIES USED**************
// include the ResponsiveAnalogRead library for analog smoothing
#include <ResponsiveAnalogRead.h>
// include the Bounce library for 'de-bouncing' switches -- removing electrical chatter as contacts settle
#include <Bounce.h>
//usbMIDI.h library is added automatically when code is compiled as a MIDI device
#include <Keyboard.h>
//Keyboard.h library is added
#include <MIDI.h>

MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI);

int fs1 = 0;
int value = 0;
int recheck = 1;
int fsset = 0;
int bank = 0;  //for bank cycling - J. Alvarez 240211
int fsVal = 0;

// ******CONSTANT VALUES********
// customize code behaviour here!
const int channel = 1;       // MIDI channel
const int A_PINS = 10;       // number of Analog PINS, HEAD UNIT, with bank
const int A_ExpPedPINS = 2;  // number of Analog PINS for Expression Pedal MIDI, with bank

const int HeadD_PINS = 8;  // number of Digital PINS FOR MIDI, HEAD UNIT with bank
const int D_PINS = 8;      // number of Digital PINS FOR MIDI, FOOT SWITCHES with bank
int checkstate;
bool pinstate[D_PINS];

const int ON_VELOCITY = 99;  // note-one velocity sent from buttons (should be 65 to  127)

// define the pins you want to use and the CC ID numbers on which to send them..
const int ANALOG_PINS[A_PINS] = { A0, A1, A2, A3, A4, A5, A6, A7, A8, A9 };  //---Knobs Assignment
const int ANALOG_PINSFOOT[A_ExpPedPINS] = { A12, A13 };                      //---Knobs Assignment

const byte CC_ANOTE[][A_PINS] = {
  // D1 values, NAME OF DIGITAL CC INPUTS are now CC_DNOTES
  { 21, 22, 23, 24, 25, 26, 27, 28, 29, 30 },  // Send D1 = 0 for the whole bank!
  { 31, 32, 33, 34, 35, 36, 37, 38, 39, 40 },
};  // this is the banks by switches matrix of CC values for the bank system

const byte CC_AexpNOTE[][A_ExpPedPINS] = {
  // D1 values, NAME OF DIGITAL CC INPUTS are now CC_DNOTES
  { 41, 42 },  // Send D1 = 0 for the whole bank!
  { 43, 44 },
};  // this is the banks by switches matrix of CC values for the bank system

// define the pins and notes for digital events
/*
const int D_PINS = 16;  // number of Digital PINS FOR MIDI, FOOT SWITCHES with bank
const int DIGITAL_PINS[D_PINS] = { 3, 4, 5, 6, 7, 8, 9, 10, 30, 31, 32, 33, 34, 35, 36, 37 };  //---Toggleswitch + Footswitch Assignment 240301
const byte CC_DNOTE[][D_PINS] = {
  // D1 values, NAME OF DIGITAL CC INPUTS are now CC_DNOTES
  { 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76 },  // Send D1 = 0 for the whole bank!
  { 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92 },
};  // this is the banks by switches matrix of CC values for the bank system
*/

const int DIGITAL_PINSHEAD[HeadD_PINS] = { 3, 4, 5, 6, 7, 8, 9, 10 };  //---Toggleswitch assignment 240301
byte CC_HeadDNOTE[][HeadD_PINS] = {
  // D1 values, NAME OF DIGITAL CC INPUTS are now CC_DNOTES
  { 45, 46, 47, 48, 49, 50, 51, 52 },  // Send D1 = 0 for the whole bank!
  { 53, 54, 55, 56, 57, 58, 59, 60 },
};  // this is the banks by switches matrix of CC values for the bank system

const int DIGITAL_PINS[D_PINS] = { 30, 31, 32, 33, 34, 35, 36, 37 };  //---Footswitch Assignment 240301
byte CC_DNOTE[][D_PINS] = {
  // D1 values, NAME OF DIGITAL CC INPUTS are now CC_DNOTES
  { 61, 62, 63, 64, 65, 66, 67, 68 },  // Send D1 = 0 for the whole bank!
  { 69, 70, 71, 72, 73, 74, 75, 76 },
};  // this is the banks by switches matrix of CC values for the bank system


const int BOUNCE_TIME = 7;  // 5 ms is usually sufficient
const boolean toggled = true;

//******VARIABLES***********
// a data array and a lagged copy to tell when MIDI changes are required
byte data[A_PINS];
byte dataLag[A_PINS];  // when lag and new are not the same then update MIDI CC value
byte dataEXP[A_ExpPedPINS];
byte dataLagEXP[A_ExpPedPINS];  // Expression Pedals

//************INITIALIZE LIBRARY OBJECTS**************

// initialize the ReponsiveAnalogRead objects
ResponsiveAnalogRead analog[]{
  { ANALOG_PINS[0], true },
  { ANALOG_PINS[1], true },
  { ANALOG_PINS[2], true },
  { ANALOG_PINS[3], true },
  { ANALOG_PINS[4], true },
  { ANALOG_PINS[5], true },
  { ANALOG_PINS[6], true },
  { ANALOG_PINS[7], true },
  { ANALOG_PINS[8], true },
  { ANALOG_PINS[9], true },
};

ResponsiveAnalogRead analogf[]{
  { ANALOG_PINSFOOT[0], true },
  { ANALOG_PINSFOOT[1], true },
};

// initialize the bounce objects
Bounce digitalh[] = {
  Bounce(DIGITAL_PINSHEAD[0], BOUNCE_TIME),
  Bounce(DIGITAL_PINSHEAD[1], BOUNCE_TIME),
  Bounce(DIGITAL_PINSHEAD[2], BOUNCE_TIME),
  Bounce(DIGITAL_PINSHEAD[3], BOUNCE_TIME),
  Bounce(DIGITAL_PINSHEAD[4], BOUNCE_TIME),
  Bounce(DIGITAL_PINSHEAD[5], BOUNCE_TIME),
  Bounce(DIGITAL_PINSHEAD[6], BOUNCE_TIME),
  Bounce(DIGITAL_PINSHEAD[7], BOUNCE_TIME),
};

Bounce digital[] = {
  Bounce(DIGITAL_PINS[0], BOUNCE_TIME),
  Bounce(DIGITAL_PINS[1], BOUNCE_TIME),
  Bounce(DIGITAL_PINS[2], BOUNCE_TIME),
  Bounce(DIGITAL_PINS[3], BOUNCE_TIME),
  Bounce(DIGITAL_PINS[4], BOUNCE_TIME),
  Bounce(DIGITAL_PINS[5], BOUNCE_TIME),
  Bounce(DIGITAL_PINS[6], BOUNCE_TIME),
  Bounce(DIGITAL_PINS[7], BOUNCE_TIME),
};

Bounce PROG_MODE = Bounce(2, 7);     // for PROGRAM MODE TOGGLE -240301 J. Alvarez
Bounce LEFTCLICK = Bounce(11, 10);   // LEFT CLICK 240229 J. Alvarez
Bounce RIGHTCLICK = Bounce(12, 10);  // RIGHT CLICK 240229 J. Alvarez
Bounce cyclebank = Bounce(13, 7);    // for bank cycling; J. Alvarez 240211
Bounce exprPdl1En = Bounce(28, 7);   // on/off switch of Expression Pedal 1; Alvarez 240301
Bounce exprPdl2En = Bounce(29, 7);   // on/off switch of Expression Pedal 2. Alvarez 240301

// *** Added for Mouse feature - 240229 J. Alvarez
const int thumbstickHorizX = A10;  // analog A10 - 24 - x
const int thumbstickHorizY = A11;  // analog A11 - 25 - y

int mouseX = 0;
int mouseY = 0;
int vertical = 0;
int horizontal = 0;
int screensizeX = 1920;
int screensizeY = 1080;

long unsigned int responsedelay = 5;
elapsedMillis moveDelay;
// parameters for reading the thumbstick
int cursorSpeed = 50;             // output speed of X or Y movement or sensitivity
int threshold = cursorSpeed / 4;  // resting threshold
int center = cursorSpeed / 2;     // resting position value
// *****************************

//************SETUP**************
void setup() {

  // loop to configure input pins and internal pullup resisters for digital section
  for (int i = 0; i < HeadD_PINS; i++) {
    pinMode(DIGITAL_PINSHEAD[i], INPUT_PULLUP);  // Head unit
  }
  for (int i = 0; i < D_PINS; i++) {
    pinMode(DIGITAL_PINS[i], INPUT_PULLUP);  //Foot Switch
  }

  Mouse.screenSize(screensizeX, screensizeY);  // configure screen size 240229 J. Alvarez

  pinMode(2, INPUT_PULLUP);   // PROGRAM MODE Toggle; J. Alvarez 240211
  pinMode(11, INPUT_PULLUP);  // LEFT CLICK 240229 J. Alvarez
  pinMode(12, INPUT_PULLUP);  // RIGHT CLICK 240229 J. Alvarez
  pinMode(13, INPUT_PULLUP);  // bank selector; original is 27; J. Alvarez 240211
  pinMode(28, INPUT_PULLUP);  // EXPRESSION PEDAL 1 240301 J. Alvarez
  pinMode(29, INPUT_PULLUP);  // EXPRESSION PEDAL 1 240301 J. Alvarez

  MIDI.begin(1);
}

//************LOOP**************
void loop() {

  while (usbMIDI.read()) {
    // controllers must call .read() to keep the queue clear even if they are not responding to MIDI
  }
  selectbank();                     //----For Bank Select, 1 switch assigned: OFF=bank 0, ON=bank 1
  getAnalogData();                  //----For Analog Knobs, 8 assigned
  getAnalogDataExpr1();             //----For Expression Pedal 1 - 240301 J. Alvarez
  getAnalogDataExpr2();             //----For Expression Pedal 2 - 240301 J. Alvarez
  getDigitalDataHFOrProgramming();  //For MIDI Toggleswitch Head unit, 8 assigned, MIDI Footswitch, 8 Assigned; disables when in program mode
  mouseclicks();                    //---For Mouse Clicks - 240229 J. Alvarez
  mousemove();                      //---For Mouse Movements - 240229 J. Alvarez
}


//************ANALOG SECTION - All Knobs**************
void getAnalogData() {
  for (int i = 0; i < A_PINS; i++) {
    // update the ResponsiveAnalogRead object every loop
    analog[i].update();  // updates the value by performing an analogRead() and calculating a responsive value based off it
    // if the repsonsive value has change, print out 'changed'
    if (analog[i].hasChanged()) {
      data[i] = analog[i].getValue() >> 3;
      if (data[i] != dataLag[i]) {
        dataLag[i] = data[i];
        usbMIDI.sendControlChange(CC_ANOTE[bank][i], data[i], channel);
      }
    }
  }
}

//************ANALOG SECTION - EXPRESSION PEDAL 1**************
void getAnalogDataExpr1() {
  exprPdl1En.update();
  if (exprPdl1En.fallingEdge()) {
    analogf[0].update();
    if (analogf[0].hasChanged()) {
      dataEXP[0] = analogf[0].getValue() >> 3;
      if (dataEXP[0] != dataLagEXP[0]) {
        dataLagEXP[0] = dataEXP[0];
        usbMIDI.sendControlChange(CC_AexpNOTE[bank][0], dataEXP[0], channel);
      }
    }
  } else {
    // do nothing
  }
}

//************ANALOG SECTION - EXPRESSION PEDAL 2**************
void getAnalogDataExpr2() {
  exprPdl2En.update();
  if (exprPdl2En.fallingEdge()) {
    analogf[1].update();
    if (analogf[1].hasChanged()) {
      dataEXP[1] = analogf[1].getValue() >> 3;
      if (dataEXP[1] != dataLagEXP[1]) {
        dataLagEXP[1] = dataEXP[1];
        usbMIDI.sendControlChange(CC_AexpNOTE[bank][1], dataEXP[1], channel);
      }
    }
  } else {
    // do nothing
  }
}


//************DIGITAL SECTION - Toggle Switch - Head Unit**************
void getDigitalDataH() {
  for (int i = 0; i < HeadD_PINS; i++) {
    digitalh[i].update();
    if (digitalh[i].fallingEdge()) {
      usbMIDI.sendControlChange(CC_HeadDNOTE[bank][i], ON_VELOCITY, channel);
    }
    // Note Off messages when each button is released
    if (digitalh[i].risingEdge()) {
      usbMIDI.sendControlChange(CC_HeadDNOTE[bank][i], 0, channel);
    }
  }
}

//************DIGITAL SECTION - FootSwitch**************
void getDigitalDataF() {
  for (int i = 0; i < D_PINS; i++) {
    digital[i].update();
    fsVal = CC_DNOTE[bank][i];
    if (digital[i].fallingEdge()) {
      usbMIDI.sendControlChange(fsVal, ON_VELOCITY, channel);
    }
    // Note Off messages when each button is released
    if (digital[i].risingEdge()) {
      usbMIDI.sendControlChange(fsVal, 0, channel);
    }
  }
}

//************BANK SELECTION - 240301 J. Alvarez**************
void selectbank() {
  cyclebank.update();
  if (cyclebank.fallingEdge()) {
    bank = 1;
  }
  if (cyclebank.risingEdge()) {
    bank = 0;
  }
}

//************Programming Section - 240301 J. Alvarez**************

void getDigitalDataHFOrProgramming() {
  PROG_MODE.update();
  if (PROG_MODE.fallingEdge()) {       //Prog Mode toggle enabled, enters footswitch programming mode
    while (!PROG_MODE.risingEdge()) {  //Stay in programming mode until switch is toggled again
      PROG_MODE.update();
      for (int i = 0; i < D_PINS; i++) {
        digital[i].update();
        if (digital[i].fallingEdge() || digital[i].risingEdge()) {
          while (!PROG_MODE.risingEdge() && (fsset == 0)) {  //wait for user to select the MIDI message to be programmed from main unit mapping. Can still toggle out of programming mode
            PROG_MODE.update();
            map2fs();  //determine which MIDI message to map to footswitch
          }
          CC_DNOTE[bank][i] = value;  //map control/program change to footswitch
          fsset = 0;                  //condition to enter loop that calls map2fs()
        }
      }
    }
  } else {
    getDigitalDataH();
    getDigitalDataF();
  }
}

void map2fs() {
  if (recheck == 1) {
    for (checkstate = 0; checkstate < HeadD_PINS; ++checkstate) {        //check and save current state of mapable inputs
      pinstate[checkstate] = digitalRead(DIGITAL_PINSHEAD[checkstate]);  //read status from toggle swtiches
    }
    recheck = 0;  //prevents constently updating saved state while waiting for user to map message
  }
  for (int i = 0; i < HeadD_PINS; i++) {
    digitalh[i].update();
    if (pinstate[checkstate] != digitalRead(DIGITAL_PINSHEAD[checkstate])) {  //execute if curernt state of input differs from saved state
      MIDI.sendControlChange(CC_HeadDNOTE[bank][i], ON_VELOCITY, channel);    //sends serial MIDI message that corresponds to usbMIDI message from the same input
      delayMicroseconds(10);                                                  //works without but helps with consistent detection? experimental
      while (!MIDI.read()) {
        //wait for serial MIDI message to be recieved before trying to fetch data. Calling MIDI.read() directly rarely fetches data for some reason
      }
      value = MIDI.getData1();  //stores control/program change value
      fsset = 1;                //condition to exit loop that calls this function
      recheck = 1;              //re-enable checking and saving state of mappable inputs next time this function is called
    }
  }
}

//************MOUSE SECTION - 240229 J. Alvarez**************

void mouseclicks() {
  LEFTCLICK.update();
  RIGHTCLICK.update();

  if (LEFTCLICK.fallingEdge()) {
    Mouse.press(MOUSE_LEFT);
  }
  if (LEFTCLICK.risingEdge()) {
    Mouse.release(MOUSE_LEFT);
  }
  if (RIGHTCLICK.fallingEdge()) {
    Mouse.press(MOUSE_RIGHT);
  }
  if (RIGHTCLICK.risingEdge()) {
    Mouse.release(MOUSE_RIGHT);
  }
}

// (Almost Directly) Copied this idea from member robsoles
// https://forum.pjrc.com/index.php?threads/guidance-for-a-mouse.29068/
// uses Mouse.move (relative) instead of Mouse.moveTo (absolute)

void mousemove() {
  if (moveDelay > responsedelay - 1) {
    moveDelay = 0;
    int xReading = readAxis(thumbstickHorizX);
    int yReading = readAxis(thumbstickHorizY);
    // if the mouse control state is active, move the mouse:
    Mouse.move(xReading, yReading, 0);  // (x, y, scroll mouse wheel)
  }
}

int readAxis(int thisAxis) {
  int reading = analogRead(thisAxis);               // read the analog input:
  reading = map(reading, 0, 1023, 0, cursorSpeed);  // map the reading from the analog input range to the output range
  int distance = reading - center;                  // if the output reading is outside from the rest position threshold, use it:
  if (abs(distance) < threshold) {
    distance = 0;
  }
  return distance;  // return the distance for this axis:
}