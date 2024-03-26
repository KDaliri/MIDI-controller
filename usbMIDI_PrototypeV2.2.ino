#include <Bounce.h>


/*************DECLARATIONS*************/

/*******MACROS*******/
#define channel 1        // MIDI channel assignment
#define a_pins 10        // number of analog pins on the head unit
#define a_fsPins 2       // number of analog pins on the footswitch unit
#define d_pins 6         // number of digital pins per unit (6 on head 6 on footswitch)
#define t_bounce 10      // Debouncing time for switches, in ms
#define on_velocity 127  // velocity for toggle ON
#define off_velocity 0   // veloctiy for toggle OFF
#define messageType 12   // program default PC/CC footswitch message toggle pin

/*******VARIABLE DECLARATIONS*******/
byte bank, checkState, progPin;  // variables named according to use
bool progSet;                    // indicates if the selected footswitch or expression pedal has been programmed

/*******ARRAY DECLARATION*******/
byte fs_Value[2][d_pins];    // saved CC/PC message per switch on footswitch unit
byte pedal_CC[2][a_fsPins];  // saved CC/PC message per expression pedal on footswitch unit
bool PC_mode[2][d_pins];     // indicates if saved MIDI message is CC or PC on footswitch

byte pot[a_pins];           // current MIDI value of knobs
byte prevPot[a_pins];       // previous MIDI value of knobs
byte pedalVal[a_fsPins];    // current MIDI value of expression pedals
byte prevPedal[a_fsPins];   // previous MIDI value of expression pedals
bool pedalState[a_fsPins];  // indicates if the expression pedal is active or inactive

const byte analogPin[a_pins] = { A0, A1, A2, A3, A4, A5, A6, A7, A8, A9 };  // analog pins for the head unit
const byte fs_analogPin[a_fsPins] = { A12, A13 };                           // analog pins for the footswitch unit
const byte digitalPin[d_pins] = { 1, 2, 3, 4, 5, 6 };                       // digital pins for the head unit
const byte fs_digitalPin[d_pins] = { 28, 29, 30, 31, 32, 33 };              // digital pins for the footswitch unit

const byte CC_knob[2][a_pins] = {
  // CC assignments for head unit knobs
  { 21, 22, 23, 24, 25, 26, 27, 28, 29, 30 },  //Bank 1
  { 31, 32, 33, 34, 35, 36, 37, 38, 39, 40 },  //Bank 2
};

const byte CC_pedal[2][a_fsPins] = {
  // CC assignments for footswitch unit pedals
  { 41, 42 },  //Bank 1
  { 43, 44 },  //Bank 2
};

const byte CC_toggle[2][d_pins] = {
  // CC assignments for head unit switches
  { 45, 46, 47, 48, 49, 50 },  // Bank 1
  { 53, 54, 55, 56, 57, 58 },  // Bank 2
};

const byte CC_footswitch[2][d_pins] = {
  // CC assignments for footswitch unit switches
  { 61, 62, 63, 64, 65, 66 },  // Bank 1
  { 69, 70, 71, 72, 73, 74 },  // Bank 2
};

const byte PC_footswitch[2][d_pins] = {
  // PC assignments for footswitch unit switches
  { 0, 1, 2, 3, 4, 5 },    // Bank 1
  { 6, 7, 8, 9, 10, 11 },  // Bank 2
};

/*******BOUNCE LIBRARY DECLARATIONS*******/
Bounce toggle[6] = {
  // declare pins from the head unit as a bounce variables
  Bounce(digitalPin[0], t_bounce),
  Bounce(digitalPin[1], t_bounce),
  Bounce(digitalPin[2], t_bounce),
  Bounce(digitalPin[3], t_bounce),
  Bounce(digitalPin[4], t_bounce),
  Bounce(digitalPin[5], t_bounce),
};

Bounce footswitch[6] = {
  // declare pins from the footswitch unit as a bounce variables. for PC/CC switches
  Bounce(fs_digitalPin[0], t_bounce),
  Bounce(fs_digitalPin[1], t_bounce),
  Bounce(fs_digitalPin[2], t_bounce),
  Bounce(fs_digitalPin[3], t_bounce),
  Bounce(fs_digitalPin[4], t_bounce),
  Bounce(fs_digitalPin[5], t_bounce),
};

Bounce pedalToggle[2] = {
  // declare pins from the footswitch unit as a bounce variables. for expression pedal ON/OFF
  Bounce(9, t_bounce),
  Bounce(10, t_bounce),
};

Bounce setBank = Bounce(0, t_bounce);    // toggle active bank or program MIDI message to active bank
Bounce progMode = Bounce(11, t_bounce);  // enter programming mode


/*************MOUSE*************/
Bounce LEFTCLICK = Bounce(8, 10);   // MOUSE LEFT CLICK
Bounce RIGHTCLICK = Bounce(7, 10);  // MOUSE RIGHT CLICK

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
int cursorSpeed = 30;             // output speed of X or Y movement or sensitivity
int threshold = cursorSpeed / 4;  // resting threshold
int center = cursorSpeed / 2;     // resting position value

/*************SETUP AND MAIN LOOP*************/

void setup() {
  // put your setup code here, to run once:
  for (progPin = 0; progPin < d_pins; progPin++) {
    pinMode(digitalPin[progPin], INPUT_PULLUP);        // initialize digital pins on the head unit as inputs
    pinMode(fs_digitalPin[progPin], INPUT_PULLUP);     // initialize digital pins on the footswitch unit as inputs
    fs_Value[0][progPin] = PC_footswitch[0][progPin];  // initial footswitch unit PC/CC switches set to PC for all of bank 1
    fs_Value[1][progPin] = PC_footswitch[1][progPin];  // initial footswitch unit PC/CC switches set to PC for all of bank 2
    PC_mode[0][progPin] = true;                        // indicate initial footswitch unit MIDI message is PC for all of bank 1
    PC_mode[1][progPin] = true;                        // indicate initial footswitch unit MIDI message is PC for all of bank 2
  }

  pinMode(0, INPUT_PULLUP);   // initialize bank toggle as input
  pinMode(7, INPUT_PULLUP);   // initialize MOUSE RIGHT CLICK
  pinMode(8, INPUT_PULLUP);   // initialize MOUSE LEFT CLICK
  pinMode(9, INPUT_PULLUP);   // initialize expression pedal1 ON/OFF toggle as input
  pinMode(10, INPUT_PULLUP);  // initialize expression pedal2 ON/OFF toggle as input
  pinMode(11, INPUT_PULLUP);  // initialize programming mode toggle as input
  pinMode(12, INPUT_PULLUP);  // initialize program CC or PC message toggle as input

  Mouse.screenSize(screensizeX, screensizeY);  // configure screen size

  if (digitalRead(0) == 1) {  // set MIDI message bank initial state based on the current bank toggle position
    bank = 0;                 // bank 1 active
    Serial.println("bank 1 active");
  } else if (digitalRead(0) == 0) {
    bank = 1;  // bank 2 active
    Serial.println("bank 2 active");
  }

  for (progPin = 0; progPin < a_pins; progPin++) {
    pot[progPin] = analogRead(analogPin[progPin]) / 8;  // set initial knob value to current knob value
    prevPot[progPin] = pot[progPin];                    // set previous knob value to current knob value. indicates knob is not being turned
  }

  for (progPin = 0; progPin < a_fsPins; progPin++) {  // set expression pedals ON/OFF initial state as the current switch state
    if (digitalRead(9 + progPin) == 1) {
      pedalState[progPin] = false;  // expression pedal(s) is off
      Serial.println((String) "pedal " + progPin + " off");
    } else if (digitalRead(9 + progPin) == 0) {
      pedalState[progPin] = true;  // expression pedal(s) is on
      Serial.println((String) "pedal " + progPin + " on");
    }
    pedal_CC[0][progPin] = CC_pedal[0][progPin];  // initial expression pedals CC message set to default value for all of bank 1
    pedal_CC[1][progPin] = CC_pedal[1][progPin];  // initial expression pedals CC message set to default value for all of bank 2
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  switches();     // check if a switch on the head unit has been toggled
  fs_switches();  // check if a switch on the footswitch unit has been toggled

  knobs();   // check if a knob has been turned
  pedals();  // check if an expression pedal has shifted position

  selectBank();  // check which bank is active

  programmingMode();  // check if programming mode has been toggled entered
  mouseclicks();      //---Check for mouse clicks
  mousemove();        //---Check for Mouse Movements

  while (usbMIDI.read()) {
  }
}


/*************PROGRAMMING SECTION*************/

/*******SELECT FOOTSWITCH*******/
void programmingMode() {
  progMode.update();  // update state of programming mode toggle

  if (progMode.fallingEdge()) {  // programming mode toggled. enter programming mode
    Serial.println("Programming mode active");
    while (!progMode.risingEdge()) {  // stay in programming mode until programming mode is toggled again
      progMode.update();              // update state of programming mode toggle

      for (progPin = 0; progPin < a_fsPins; progPin++) {
        pedalToggle[progPin].update();  // update state of expression pedal ON/OFF

        if (pedalToggle[progPin].fallingEdge() || pedalToggle[progPin].risingEdge()) {  // expression pedal ON/OFF has been toggled. indicates this pedal is being programmed
          Serial.println("pedal selected");
          map2pedal();  // turn a knob. its MIDI message will be mapped to the pedal being programmed
        }
      }

      for (progPin = 0; progPin < d_pins; progPin++) {
        footswitch[progPin].update();  // update state of switches on the footswitch unit
        toggle[progPin].update();      // update state of switches on the head unit

        if (footswitch[progPin].fallingEdge() || footswitch[progPin].risingEdge()) {  // switch on the footswitch unit is toggled. indicates this switch is being programmed
          Serial.println("fs selected");
          map2fs();  // toggle a switch. its MIDI message will be mapped to the switch being programmed
        } else {
          while (usbMIDI.read()) {
          }
        }
      }
    }
  }
}

/*******MAP PEDAL MESSAGE*******/
void map2pedal() {
  for (checkState = 0; checkState < a_pins; checkState++) {
    pot[checkState] = analogRead(analogPin[checkState]) / 8;  // save current knob value
    prevPot[checkState] = pot[checkState];                    // set previous knob value to current knob value. indicates knob is not being turned
  }
  progSet = false;  // indicates the expression pedal has not been mapped

  while (progSet == false) {  // wait for user to map pedal
    selectBank();             // check which bank is active

    for (checkState = 0; checkState < a_fsPins; checkState++) {
      pedalToggle[checkState].update();  // update state of expression pedal ON/OFF

      if (pedalToggle[checkState].risingEdge() || pedalToggle[checkState].fallingEdge()) {  // expression pedal ON/OFF is toggled. program MIDI message of this pedal
        pedal_CC[bank][progPin] = CC_pedal[bank][checkState];                               // save CC message of selected pedal and active bank to pedal being programmed
        progSet = true;                                                                     // indicates pedal has been mapped
        Serial.println("pedal default");
      }
    }
    for (checkState = 0; checkState < a_pins; checkState++) {
      pot[checkState] = analogRead(analogPin[checkState]) / 8;

      if (pot[checkState] > prevPot[checkState] + 1 || pot[checkState] < prevPot[checkState] - 1) {  // knob on head unit is turned. program MIDI message of this knob
        pedal_CC[bank][progPin] = CC_knob[bank][checkState];                                         // save CC message of selected knob and active bank to pedal being programmed
        progSet = true;                                                                              // indicates pedal has been mapped
        Serial.println("pedal mapped");
      }
    }
    progMode.update();  // update state of programming mode toggle

    for (checkState = 0; checkState < d_pins; checkState++) {
      toggle[checkState].update();      // update state of switches on the head unit
      footswitch[checkState].update();  // update state of switches on the footswitch unit
    }

    if (progMode.risingEdge()) {  // programming mode toggled
      Serial.println("pedal not changed");
      break;  // exits programming mode without programming pedal
    } else {
      while (usbMIDI.read()) {
      }
    }
  }
}

/*******MAP SWITCH MESSAGE*******/
void map2fs() {
  progSet = false;  // indicates the footswitch has not been mapped

  while (progSet == false) {  // wait for user to map footswitch
    selectBank();             // check which bank is active

    for (checkState = 0; checkState < d_pins; checkState++) {
      toggle[checkState].update();      // update state of switches on the head unit
      footswitch[checkState].update();  // update state of switches on the footswitch unit

      if (toggle[checkState].fallingEdge() || toggle[checkState].risingEdge()) {  // switch from head unit is toggled. program MIDI message of this switch
        fs_Value[bank][progPin] = CC_toggle[bank][checkState];                    // save CC message of selected switch and active bank to switch being programmed
        PC_mode[bank][progPin] = false;                                           // indicate saved MIDI message is not PC
        Serial.println("CC toggle set");
        progSet = true;  // indicates footswitch has been mapped
      }

      else if (footswitch[checkState].fallingEdge() || footswitch[checkState].risingEdge()) {  // switch from footswitch unit is toggled. program MIDI message of this switch

        if (digitalRead(messageType) == 1) {                          // indicates PC message should be programmed
          fs_Value[bank][progPin] = PC_footswitch[bank][checkState];  // save PC message of selected switch and active bank to switch being programmed
          PC_mode[bank][progPin] = true;                              // indicate saved MIDI message is PC
          Serial.println("PC fs set");
        } else if (digitalRead(messageType) == 0) {                   // indicates CC message should be programmed
          fs_Value[bank][progPin] = CC_footswitch[bank][checkState];  // save CC message of selected switch and active bank to switch being programmed
          PC_mode[bank][progPin] = false;                             // indicate saved MIDI message is not PC
          Serial.println("CC fs set");
        }
        progSet = true;  // indicates footswitch has been mapped
      }
    }
    progMode.update();        // update state of programming mode toggle
    pedalToggle[0].update();  // update state of expression pedal1 ON/OFF
    pedalToggle[1].update();  // update state of expression pedal2 ON/OFF

    if (progMode.risingEdge()) {  // programming mode toggled
      Serial.println("fs not changed");
      break;  // exits programming mode without programming footswitch
    } else {
      while (usbMIDI.read()) {
      }
    }
  }
}

/*************DIGITAL SECTION*************/

/*******HEAD UNIT*******/
void switches() {
  for (checkState = 0; checkState < d_pins; checkState++) {
    toggle[checkState].update();  // update state of switches on the head unit

    if (toggle[checkState].fallingEdge()) {                                          // switch on head unit toggled
      usbMIDI.sendControlChange(CC_toggle[bank][checkState], on_velocity, channel);  // send CC message of toggled switch from active bank
      Serial.println((String) "toggle CC " + CC_toggle[bank][checkState] + " on");
    } else if (toggle[checkState].risingEdge()) {                                     // switch on head unit toggled
      usbMIDI.sendControlChange(CC_toggle[bank][checkState], off_velocity, channel);  // send CC message of toggled switch from active bank
      Serial.println((String) "toggle CC " + CC_toggle[bank][checkState] + " off");
    }
  }
}

/*******FOOTSWITCH UNIT*******/
void fs_switches() {
  for (checkState = 0; checkState < d_pins; checkState++) {
    footswitch[checkState].update();  // update state of switches on the footswitch unit

    if (footswitch[checkState].fallingEdge()) {                          // switch on footswitch unit toggled
      if (PC_mode[bank][checkState] == true) {                           // indicate saved MIDI message is PC
        usbMIDI.sendProgramChange(fs_Value[bank][checkState], channel);  // send PC message of toggled switch from active bank
        Serial.println((String) "fs PC " + fs_Value[bank][checkState] + " sent");
      } else if (PC_mode[bank][checkState] == false) {                                // indicate saved MIDI message is CC
        usbMIDI.sendControlChange(fs_Value[bank][checkState], on_velocity, channel);  // send CC message of toggled switch from active bank
        Serial.println((String) "fs CC " + fs_Value[bank][checkState] + " on");
      }
    } else if (footswitch[checkState].risingEdge()) {                    // switch on footswitch unit toggled
      if (PC_mode[bank][checkState] == true) {                           // indicate saved MIDI message is PC
        usbMIDI.sendProgramChange(fs_Value[bank][checkState], channel);  // send PC message of toggled switch from active bank
        Serial.println((String) "fs PC " + fs_Value[bank][checkState] + " sent");
      } else if (PC_mode[bank][checkState] == false) {                                 // indicate saved MIDI message is CC
        usbMIDI.sendControlChange(fs_Value[bank][checkState], off_velocity, channel);  // send CC message of toggled switch from active bank
        Serial.println((String) "fs CC " + fs_Value[bank][checkState] + " off");
      }
    }
  }
}

/*******BANK SELECTION********/
void selectBank() {
  setBank.update();  // update state of switches on the footswitch unit

  if (setBank.fallingEdge()) {  // bank select toggled
    bank = 1;                   // bank 2 active
    Serial.println("bank 2 active");
  } else if (setBank.risingEdge()) {  // bank select toggled
    bank = 0;                         // bank 1 active
    Serial.println("bank 1 active");
  }
}


/*************ANALOG SECTION*************/

/*******HEAD UNIT*******/
void knobs() {
  for (checkState = 0; checkState < a_pins; checkState++) {
    pot[checkState] = analogRead(analogPin[checkState]) / 8;  // save current value of knobs

    if (pot[checkState] > prevPot[checkState] + 1 || pot[checkState] < prevPot[checkState] - 1) {  // current value is more than +1/-1. the knob is being turned
      pot[checkState] = analogRead(analogPin[checkState]) / 8;                                     // recheck value for accuracy
      usbMIDI.sendControlChange(CC_knob[bank][checkState], pot[checkState], channel);              // send CC message from active bank and and value of the knob being turned
      Serial.println((String) "pot value: " + pot[checkState]);
      prevPot[checkState] = pot[checkState];  // current value is now the previous value. used to determine when the knob has stopped turning
    }
  }
}

/*******FOOTSWITCH UNIT*******/
void pedals() {
  for (checkState = 0; checkState < a_fsPins; checkState++) {
    pedalToggle[checkState].update();  // update state of expression pedal ON/OFF

    if (pedalToggle[checkState].risingEdge()) {
      pedalState[checkState] = false;  // pedal toggled OFF
      Serial.println((String) "pedal " + progPin + " off");
    } else if (pedalToggle[checkState].fallingEdge()) {
      pedalState[checkState] = true;  // pedal toggled ON
      Serial.println((String) "pedal " + progPin + " on");
    }
    if (pedalState[checkState] == true) {                               // read the value of the pedal if toggled ON
      pedalVal[checkState] = analogRead(fs_analogPin[checkState]) / 8;  // save current value of pedal

      if (pedalVal[checkState] > prevPedal[checkState] + 1 || pedalVal[checkState] < prevPedal[checkState] - 1) {  // current value is more than +1/-1. the pedal is being acted upon
        pedalVal[checkState] = analogRead(fs_analogPin[checkState]) / 8;                                           // recheck value for accuracy
        usbMIDI.sendControlChange(pedal_CC[bank][checkState], pedalVal[checkState], channel);                      // send CC message from active bank and and value of the pedal being acted upon
        Serial.println((String) "pedal value: " + pedalVal[checkState]);
        prevPedal[checkState] = pedalVal[checkState];  // current value is now the previous value. used to determine when the pedal is no longer being acted upon
      }
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

