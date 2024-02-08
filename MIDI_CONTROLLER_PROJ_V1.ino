
/* Use arrays to manage lists of knobs/pots and pushbuttons.

   By Leif Oddson
   https://forum.pjrc.com/threads/45376

   This more complex example demonstrates how to use arrays to
   manage a larger number of inputs, without duplicating your
   code for every signal.

   You must select MIDI from the "Tools > USB Type" menu

   This example code is in the public domain.

   Jan 21, 2024 - Version 1 - J. Alvarez
*/


//************LIBRARIES USED**************
// include the ResponsiveAnalogRead library for analog smoothing
#include <ResponsiveAnalogRead.h>
// include the Bounce library for 'de-bouncing' switches -- removing electrical chatter as contacts settle
#include <Bounce.h>
//usbMIDI.h library is added automatically when code is compiled as a MIDI device
#include <Keyboard.h>
//Keyboard.h library is added
//#include <EEPROM.h>

// ******CONSTANT VALUES********
// customize code behaviour here!
const int channel = 1;       // MIDI channel
const int A_PINS = 6;        // number of Analog PINS
const int D_PINS = 3;        // number of Digital PINS FOR MIDI
const int ON_VELOCITY = 99;  // note-one velocity sent from buttons (should be 65 to  127)
//int chnl;                    // MIDI channel
//const int delayL = 5;        // > delay can reduce number of MIDI msgs - can be good or bad??
// higher delay likely advantage if using lower threshold to limit repeats at same CC
int tabs = 2;

// define the pins you want to use and the CC ID numbers on which to send them..
const int ANALOG_PINS[A_PINS] = { A0, A1, A2, A3, A4, A5 };
const int CCID[A_PINS] = { 21, 22, 23, 24, 25, 26 };

// define the pins and notes for digital events
const int DIGITAL_PINS[D_PINS] = { 0, 1, 2 };
const int note[D_PINS] = { 60, 61, 62 };
const int BOUNCE_TIME = 7;  // 5 ms is usually sufficient
const boolean toggled = true;
//int toggled[3] = { false, false, false };        // toggle indicates when toggle behavior in effect
//int mem[14] = { 1, 4, 7, 64, 0, 65, 0, 66, 0 };  // an array of sysEx bytes, + defaults until EEPROM is loaded
//const int CCID_D[D_PINS] = { 60, 61, 62 };


//******VARIABLES***********
// a data array and a lagged copy to tell when MIDI changes are required
byte data[A_PINS];
byte dataLag[A_PINS];  // when lag and new are not the same then update MIDI CC value


//************INITIALIZE LIBRARY OBJECTS**************
// not sure if there is a better way... some way run a setup loop on global array??
// use comment tags to comment out unused portions of array definitions

// initialize the ReponsiveAnalogRead objects
ResponsiveAnalogRead analog[]{
  { ANALOG_PINS[0], true },
  { ANALOG_PINS[1], true },
  { ANALOG_PINS[2], true },
  { ANALOG_PINS[3], true },
  { ANALOG_PINS[4], true },
  { ANALOG_PINS[5], true } /*,
  {ANALOG_PINS[6],true},
  {ANALOG_PINS[7],true},*/
};

// initialize the bounce objects
Bounce digital[] = {
  Bounce(DIGITAL_PINS[0], BOUNCE_TIME),
  Bounce(DIGITAL_PINS[1], BOUNCE_TIME),
  Bounce(DIGITAL_PINS[2], BOUNCE_TIME),
  /*Bounce(DIGITAL_PINS[3], BOUNCE_TIME),
  Bounce(DIGITAL_PINS[4], BOUNCE_TIME),
  Bounce(DIGITAL_PINS[5], BOUNCE_TIME),
  Bounce(DIGITAL_PINS[6], BOUNCE_TIME),
  Bounce(DIGITAL_PINS[7], BOUNCE_TIME),
  Bounce(DIGITAL_PINS[8], BOUNCE_TIME),*/
};

Bounce button3 = Bounce(3, 5);  // keybaord
Bounce button4 = Bounce(4, 5);  // keybaord
Bounce button5 = Bounce(5, 5);  // keybaord
Bounce button6 = Bounce(6, 5);  // keybaord

//************SETUP**************
void setup() {
  // loop to configure input pins and internal pullup resisters for digital section
  for (int i = 0; i < D_PINS; i++) {
    pinMode(DIGITAL_PINS[i], INPUT_PULLUP);
  }
  pinMode(3, INPUT_PULLUP);  // keyboard stroke
  pinMode(4, INPUT_PULLUP);  // keyboard stroke
  pinMode(5, INPUT_PULLUP);  // keyboard stroke
  pinMode(6, INPUT_PULLUP);  // keyboard stroke
}

//************LOOP**************
void loop() {
  getAnalogData();
  getDigitalData();
  while (usbMIDI.read()) {
    // controllers must call .read() to keep the queue clear even if they are not responding to MIDI
  }

  button3.update();
  button4.update();
  button5.update();
  button6.update();

  if (button3.fallingEdge()) {
    Keyboard.press(KEY_UP);
    Keyboard.release(KEY_UP);
  }
  if (button4.fallingEdge()) {
    Keyboard.press(KEY_DOWN);
    Keyboard.release(KEY_DOWN);
  }
  if (button5.fallingEdge()) {
    //Keyboard.press(KEY_ENTER);
    //Keyboard.release(KEY_ENTER);
    Keyboard.set_modifier(0);
    Keyboard.send_now();
  }

  if (button6.fallingEdge()) {
    Keyboard.set_modifier(MODIFIERKEY_ALT);
    Keyboard.send_now();

    //delay(250);
    //Keyboard.set_modifier(0);
    //Keyboard.send_now();
    tab();
    //delay(250);
    //Keyboard.press(KEY_TAB);
    //Keyboard.release(KEY_TAB);
    delay(250);
    // Keyboard.set_modifier(0);
    //Keyboard.send_now();
  }

  /*if (button6.risingEdge()) {
  }
  if (button6.fallingEdge()) {
    Keyboard.press(KEY_TAB);
    Keyboard.release(KEY_TAB);
  }*/
}


//************ANALOG SECTION**************
void getAnalogData() {
  for (int i = 0; i < A_PINS; i++) {
    // update the ResponsiveAnalogRead object every loop
    analog[i].update();
    // if the repsonsive value has change, print out 'changed'
    if (analog[i].hasChanged()) {
      data[i] = analog[i].getValue() >> 3;
      if (data[i] != dataLag[i]) {
        dataLag[i] = data[i];
        usbMIDI.sendControlChange(CCID[i], data[i], channel);
      }
    }
  }
}

//************DIGITAL SECTION**************
void getDigitalData() {
  for (int i = 0; i < D_PINS; i++) {
    digital[i].update();
    if (digital[i].fallingEdge()) {
      usbMIDI.sendNoteOn(note[i], ON_VELOCITY, channel);
    }
    // Note Off messages when each button is released
    if (digital[i].risingEdge()) {
      usbMIDI.sendNoteOff(note[i], 0, channel);
    }
  }
}

void tab() {
  switch (tabs) {
    case 2:
      for (int x = 1; x < tabs; ++x) {
        Keyboard.press(KEY_TAB);
        Keyboard.release(KEY_TAB);
      }
      tabs = 3;
      break;
    case 3:
      for (int x = 1; x < tabs; ++x) {
        Keyboard.press(KEY_TAB);
        Keyboard.release(KEY_TAB);
      }
      tabs = 2;
      break;
  }
}
