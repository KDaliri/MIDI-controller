
/* Use arrays to manage lists of knobs/pots and pushbuttons.

   By Leif Oddson
   https://forum.pjrc.com/threads/45376

   Teensy 4.1

   Jan 21, 2024 - Version 1 - J. Alvarez
   Jan 31, 2024 - Version 2 - J. Alvarez - Added SSD1306 OLED Display Capabilities and ENTER keyboard stroke
   Feb 11, 2024 - Version 3 - J. Alvarez - Added Bank and Program Change functions
   Feb 29, 2024 - Version 4 - J. Alvarez - Added Mouse Feature
*/

//************LIBRARIES USED**************
// include the ResponsiveAnalogRead library for analog smoothing
#include <ResponsiveAnalogRead.h>
// include the Bounce library for 'de-bouncing' switches -- removing electrical chatter as contacts settle
#include <Bounce.h>
//usbMIDI.h library is added automatically when code is compiled as a MIDI device
#include <Keyboard.h>
//Keyboard.h library is added
//#include <MIDI.h>

/*
3
9
14-15
20-31
85-90
102-119
*/

#include <U8g2lib.h>  //Added 240131
//---Added 240131
#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif
//---

//MIDI_CREATE_INSTANCE(HardwareSerial, Serial2, MIDI)

U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, /* clock=*/24, /* data=*/25, /* reset=*/U8X8_PIN_NONE);  // All Boards without Reset of the Display, Added 240131

int LEDoff, LEDnum;
int preset = 1;
/*
int fs1 = 0;
int value = 0;
int recheck = 1;
int fsset = 0;
int checkstate;
*/
// ******CONSTANT VALUES********
// customize code behaviour here!
const int channel = 1;       // MIDI channel
const int A_PINS = 6;        // number of Analog PINS
const int D_PINS = 6;        // number of Digital PINS FOR MIDI
const int ON_VELOCITY = 99;  // note-one velocity sent from buttons (should be 65 to  127)
int tabs = 2;


// *** Added for Mouse feature - 240229 J. Alvarez
const int thumbstickHorizX = A17;  // analog A17 - x
const int thumbstickHorizY = A16;  // analog A16 - y

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

// define the pins you want to use and the CC ID numbers on which to send them..
const int ANALOG_PINS[A_PINS] = { A0, A1, A2, A3, A4, A5 };
// define the pins and notes for digital events
const int DIGITAL_PINS[D_PINS] = { 0, 1, 2, 3, 4, 5 };  //---Added Digital pins 3 to 5, 240131
int bank = 0;                                           //for bank cycling - J. Alvarez 240211

const byte CC_ANOTE[][A_PINS] = {
  // D1 values, NAME OF DIGITAL CC INPUTS are now CC_DNOTES
  { 21, 22, 23, 24, 25, 26 },  // Send D1 = 0 for the whole bank!
  { 27, 28, 29, 30, 31, 32 },
  { 33, 34, 35, 36, 37, 38 },
};  // this is the banks by switches matrix of CC values for the bank system

const byte CC_DNOTE[][D_PINS] = {
  // D1 values, NAME OF DIGITAL CC INPUTS are now CC_DNOTES
  { 60, 61, 62, 63, 64, 65 },  // Send D1 = 0 for the whole bank!
  { 66, 67, 68, 69, 70, 71 },
  { 72, 73, 74, 75, 76, 77 },
};  // this is the banks by switches matrix of CC values for the bank system

const int BOUNCE_TIME = 7;  // 5 ms is usually sufficient
const boolean toggled = true;

//**Display--Added 240131
u8g2_uint_t offset;                                          // current offset for the scrolling text
u8g2_uint_t width;                                           // pixel width of the scrolling text (must be lesser than 128 unless U8G2_16BIT is defined
const char *text = "MIDI CONTROLLER - CENTENNIAL COLLEGE ";  // scroll this text from right to left

//******VARIABLES***********
// a data array and a lagged copy to tell when MIDI changes are required
byte data[A_PINS];
byte dataLag[A_PINS];  // when lag and new are not the same then update MIDI CC value

//************INITIALIZE LIBRARY OBJECTS**************

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

ResponsiveAnalogRead analogX(A17, true);
ResponsiveAnalogRead analogY(A16, true);

// initialize the bounce objects
Bounce digital[] = {
  Bounce(DIGITAL_PINS[0], BOUNCE_TIME),
  Bounce(DIGITAL_PINS[1], BOUNCE_TIME),
  Bounce(DIGITAL_PINS[2], BOUNCE_TIME),
  Bounce(DIGITAL_PINS[3], BOUNCE_TIME),
  Bounce(DIGITAL_PINS[4], BOUNCE_TIME),
  Bounce(DIGITAL_PINS[5], BOUNCE_TIME),
  /*Bounce(DIGITAL_PINS[6], BOUNCE_TIME),
  Bounce(DIGITAL_PINS[7], BOUNCE_TIME),
  Bounce(DIGITAL_PINS[8], BOUNCE_TIME),*/
};

Bounce UP_KEY = Bounce(8, 5);        // keybaord
Bounce DOWN_KEY = Bounce(9, 5);      // keybaord
Bounce ENTER_KEY = Bounce(10, 5);    // keybaord
Bounce ALTtTAB_KEY = Bounce(11, 5);  // keybaord
Bounce RELEASE_KEY = Bounce(12, 5);  // keybaord

Bounce MIDI_PCHNGE = Bounce(7, 7);  // for Program change MIDI SIGNAL sending
Bounce cyclebank = Bounce(29, 7);   // for bank cycling; original is 27; J. Alvarez 240211
//Bounce PROG_MODE = Bounce(30, 7);   // for PROGRAM MODE TOGGLE

Bounce LEFTCLICK = Bounce(31, 10);   // LEFT CLICK 240229 J. Alvarez
Bounce RIGHTCLICK = Bounce(30, 10);  // RIGHT CLICK 240229 J. Alvarez

//************SETUP**************
void setup() {

  // loop to configure input pins and internal pullup resisters for digital section
  for (int i = 0; i < D_PINS; i++) {
    pinMode(DIGITAL_PINS[i], INPUT_PULLUP);
  }
  Mouse.screenSize(screensizeX, screensizeY);  // configure screen size 240229 J. Alvarez

  pinMode(7, INPUT_PULLUP);   // Preset Program Change Midi button; J. Alvarez 240211
  pinMode(8, INPUT_PULLUP);   // keyboard stroke UP
  pinMode(9, INPUT_PULLUP);   // keyboard stroke DOWN
  pinMode(10, INPUT_PULLUP);  // keyboard stroke ENTER Added 240131
  pinMode(11, INPUT_PULLUP);  // keyboard stroke ALT+TAB
  pinMode(12, INPUT_PULLUP);  // keyboard stroke RELEASE MODIFIER
  pinMode(29, INPUT_PULLUP);  // bank selector; original is 27; J. Alvarez 240211

  pinMode(20, OUTPUT);     //LED for PC0 message -- Added 240211
  pinMode(21, OUTPUT);     //LED for PC1 message -- Added 240211
  pinMode(22, OUTPUT);     //LED for PC2 message -- Added 240211
  pinMode(23, OUTPUT);     //LED for PC3 message -- Added 240211
  digitalWrite(20, HIGH);  //LED initial state for PC0 message -- Added 240211
  digitalWrite(21, LOW);   //LED initial state for PC1 message -- Added 240211
  digitalWrite(22, LOW);   //LED initial state for PC2 message -- Added 240211
  digitalWrite(23, LOW);   //LED initial state for PC3 message -- Added 240211

  pinMode(26, OUTPUT);     //LED for CC message bank 1 -- Added 240211
  pinMode(27, OUTPUT);     //LED for CC message bank 2 -- Added 240211
  pinMode(28, OUTPUT);     //LED for CC message bank 3 -- Added 240211
  digitalWrite(26, HIGH);  //LED initial state for CC message bank 1 -- Added 240211
  digitalWrite(27, LOW);   //LED initial state for CC message bank 2 -- Added 240211
  digitalWrite(28, LOW);   //LED initial state for CC message bank 3 -- Added 240211

  //pinMode(30, INPUT_PULLUP);  // PROGRAM MODE Toggle; J. Alvarez 240211
  //pinMode(31, OUTPUT);        //LED indicator for PROGRAM MODE -- Added 240211
  //digitalWrite(31, LOW);      //LED initial state for PROGRAM MODE indicator -- Added 240211

  pinMode(31, INPUT_PULLUP);  // LEFT CLICK 240229 J. Alvarez
  pinMode(30, INPUT_PULLUP);  // RIGHT CLICK 240229 J. Alvarez

  u8g2.begin();        //Added 240131
  u8g2.clearBuffer();  // clear the internal memory, Added 240131

  u8g2.setFontMode(0);                 // enable transparent mode, which is faster
  u8g2.setFont(u8g2_font_04b_03b_tr);  // choose a suitable font

  u8g2.drawStr(0, 5, "Controller ACTIVE");  // write something to the internal memory
  u8g2.sendBuffer();
  delay(500);
  u8g2.drawStr(100, 5, "Bank 1");  // write something to the internal memory
  u8g2.sendBuffer();               // transfer internal memory to the display
  delay(500);
  //MIDI.begin(1);
}

//************LOOP**************
void loop() {

  while (usbMIDI.read()) {
    // controllers must call .read() to keep the queue clear even if they are not responding to MIDI
  }
  keystrokes();         //---- for Keystrokes ENTER, UP, DOWN, ALT-TAB, and RELEASE, 4 buttons
  cyclebank.update();   //-----for Bank; original is 27, 1 button; J. Alvarez 240211
  cyclebanklighters();  //-----LED Bank indicators, 3 LED Indicators; J.Alvarez
  getAnalogData();      //----For Analog Knobs, 6 assigned
  getDigitalData();     //----For MIDI TOGGLE, 6 assigned
  recall();             //---For preset MIDI Program change, 1 button, 4 LED indicators
  mouseclicks();        //---For Mouse Clicks - 240229 J. Alvarez
  mousemove();          //---For Mouse Movements - 240229 J. Alvarez
}


//************ANALOG SECTION**************
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

//************DIGITAL SECTION**************
void getDigitalData() {
  for (int i = 0; i < D_PINS; i++) {
    digital[i].update();
    if (digital[i].fallingEdge()) {
      usbMIDI.sendNoteOn(CC_DNOTE[bank][i], ON_VELOCITY, channel);
    }
    // Note Off messages when each button is released
    if (digital[i].risingEdge()) {
      usbMIDI.sendNoteOff(CC_DNOTE[bank][i], 0, channel);
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

void scrollerBoard() {
  //------Added 240131
  u8g2_uint_t x;

  u8g2.firstPage();
  do {

    // draw the scrolling text at current offset
    x = offset;
    u8g2.setFont(u8g2_font_inb30_mr);      // set the target font
    do {                                   // repeated drawing of the scrolling text...
      u8g2.drawUTF8(x, 30, text);          // draw the scolling text
      x += width;                          // add the pixel width of the scrolling text
    } while (x < u8g2.getDisplayWidth());  // draw again until the complete display is filled

    u8g2.setFont(u8g2_font_inb16_mr);  // draw the current pixel width
    u8g2.setCursor(0, 58);
    u8g2.print("ACTIVE");  // this value must be lesser than 128 unless U8G2_16BIT is set

  } while (u8g2.nextPage());

  offset -= 1;  // scroll by one pixel
  if ((u8g2_uint_t)offset < (u8g2_uint_t)-width)
    offset = 0;  // start over again

  delay(10);
  //------Added 240131
}
/*
void programming() {
  if (program.fallingEdge()) {       //Toggle switch enters footswitch programming mode
    digitalWrite(31, HIGH);          //LED indicates controller is in programming mode
    while (!program.risingEdge()) {  //Stay in programming mode until switch is toggled again
      digitalWrite(26, LOW);         //all active footswitch LEDs turn off
      program.update();
      stomp2.update();
      if (stomp2.fallingEdge() || stomp2.risingEdge()) {  //program footswitch1
        digitalWrite(26, HIGH);                           //LED indicates footswitch1 is being programmed
        while (!program.risingEdge() && (fsset == 0)) {   //wait for user to select the MIDI message to be programmed from main unit mapping. Can still toggle out of programming mode
          program.update();
          map2fs();  //determine which MIDI message to map to footswitch
        }
        fs1 = value;  //map control/program change to footswitch1
        fsset = 0;    //condition to enter loop that calls map2fs()
      }
    }
  }
  digitalWrite(31, LOW);  //LED indicates controller is exiting programming mode
}

*/
void keystrokes() {
  UP_KEY.update();  //------Added 240131
  DOWN_KEY.update();
  ENTER_KEY.update();
  ALTtTAB_KEY.update();
  RELEASE_KEY.update();
  //------Added 240131
  if (ENTER_KEY.fallingEdge()) {
    Keyboard.press(KEY_ENTER);
    Keyboard.release(KEY_ENTER);
    //u8g2.begin();
    u8g2.clearBuffer();                  // clear the internal memory
    u8g2.setFont(u8g2_font_ncenB08_tr);  // choose a suitable font
    u8g2.drawUTF8(28, 31, "ENTER");      // write something to the internal memory
    u8g2.sendBuffer();                   // transfer internal memory to the display
    delay(100);
  }
  //------Added 240131

  if (UP_KEY.fallingEdge()) {
    Keyboard.press(KEY_UP);
    Keyboard.release(KEY_UP);
    //u8g2.begin();
    u8g2.clearBuffer();                  // clear the internal memory
    u8g2.setFont(u8g2_font_ncenB08_tr);  // choose a suitable font
    u8g2.drawUTF8(31, 31, "UP");         // write something to the internal memory
    u8g2.sendBuffer();                   // transfer internal memory to the display
    delay(100);
  }

  if (DOWN_KEY.fallingEdge()) {
    Keyboard.press(KEY_DOWN);
    Keyboard.release(KEY_DOWN);
    u8g2.clearBuffer();                  // clear the internal memory
    u8g2.setFont(u8g2_font_ncenB08_tr);  // choose a suitable font
    u8g2.drawUTF8(30, 31, "DOWN");       // write something to the internal memory
    u8g2.sendBuffer();                   // transfer internal memory to the display
    delay(100);
  }

  if (RELEASE_KEY.fallingEdge()) {
    Keyboard.set_modifier(0);
    Keyboard.send_now();
    //u8g2.begin();
    u8g2.clearBuffer();                          // clear the internal memory
    u8g2.setFont(u8g2_font_ncenB08_tr);          // choose a suitable font
    u8g2.drawUTF8(0, 21, "ALt-Tab - Released");  // write something to the internal memory
    u8g2.sendBuffer();                           // transfer internal memory to the display
    delay(100);
  }

  if (ALTtTAB_KEY.fallingEdge()) {
    Keyboard.set_modifier(MODIFIERKEY_ALT);
    Keyboard.send_now();
    u8g2.clearBuffer();                          // clear the internal memory
    u8g2.setFont(u8g2_font_ncenB08_tr);          // choose a suitable font
    u8g2.drawUTF8(0, 21, "ALt-Tab - Released");  // write something to the internal memory
    u8g2.sendBuffer();                           // transfer internal memory to the display
    tab();
    delay(250);
  }
}

void recall() {
  MIDI_PCHNGE.update();
  if (MIDI_PCHNGE.fallingEdge()) {
    usbMIDI.sendProgramChange(preset, channel);  //ProgramChange typically used to recall saved presets. Preset value corresponds to ProgramChange value
    LEDnum = preset + 20;
    digitalWrite(LEDnum, !digitalRead(LEDnum));  //For LED tracking of ProgramChange value. Turns LED that corresponds to the sent ProgramChange value on; added to 20 since led starts at 20 J. Alvarez 240211
    LEDoff = LEDnum - 1;                         //Saves pin value of previous ProgramChange message; added 20 since LED starts at 20 J. Alvarez 240211
    if (preset == 0) {                           //fix LEDoff value after resetting ProgramChange value to initial state
      LEDoff = 23;                               //made 23 since LED ends at 23 J. Alvarez 240211
    }
    digitalWrite(LEDoff, !digitalRead(LEDoff));  //turns LED that corresponds to previous ProgramChange value off
    ++preset;                                    //Cycle to one of 4 ProgramChange values each time the function is called
    if (preset == 4) {                           //Reset ProgramChange value to initial state
      preset = 0;
    }
  }
  //u8g2.clearBuffer();                  // clear the internal memory
  u8g2.setFont(u8g2_font_04b_03b_tr);  // choose a suitable font
  u8g2.setCursor(90, 12);
  u8g2.print("PRESET: ");  // this value must be lesser than 128 unless U8G2_16BIT is set
  u8g2.sendBuffer();       // transfer internal memory to the display
  //delay(10);
  u8g2.setCursor(123, 12);
  u8g2.print(preset);  // this value must be lesser than 128 unless U8G2_16BIT is set
  u8g2.sendBuffer();   // transfer internal memory to the display
  //delay(10);
}


void cyclebanklighters() {
  if (cyclebank.fallingEdge()) {  //used momentary button for bank cycling
    ++bank;                       //cyle ControlChange message bank. Each bank changes the ControlChange message number sent by the inputs
    switch (bank) {               //LED tracking of bank selection. Also used to reset bank variable
      case 0:
        digitalWrite(26, HIGH);
        digitalWrite(27, LOW);
        digitalWrite(28, LOW);
        u8g2.setFontMode(0);                 // enable transparent mode, which is faster
        u8g2.setFont(u8g2_font_04b_03b_tr);  // choose a suitable font
        u8g2.drawStr(100, 5, "Bank 1");      // write something to the internal memory
        u8g2.sendBuffer();                   // transfer internal memory to the display
        delay(100);
        break;
      case 1:
        digitalWrite(26, LOW);
        digitalWrite(27, HIGH);
        digitalWrite(28, LOW);
        u8g2.setFontMode(0);                 // enable transparent mode, which is faster
        u8g2.setFont(u8g2_font_04b_03b_tr);  // choose a suitable font
        u8g2.drawStr(100, 5, "Bank 2");      // write something to the internal memory
        u8g2.sendBuffer();                   // transfer internal memory to the display
        delay(100);
        break;
      case 2:
        digitalWrite(26, LOW);
        digitalWrite(27, LOW);
        digitalWrite(28, HIGH);
        u8g2.setFontMode(0);                 // enable transparent mode, which is faster
        u8g2.setFont(u8g2_font_04b_03b_tr);  // choose a suitable font
        u8g2.drawStr(100, 5, "Bank 3");      // write something to the internal memory
        u8g2.sendBuffer();                   // transfer internal memory to the display
        delay(100);
        break;
      case 3:
        bank = 0;
        digitalWrite(26, HIGH);
        digitalWrite(27, LOW);
        digitalWrite(28, LOW);
        u8g2.setFontMode(0);                 // enable transparent mode, which is faster
        u8g2.setFont(u8g2_font_04b_03b_tr);  // choose a suitable font
        u8g2.drawStr(100, 5, "Bank 1");      // write something to the internal memory
        u8g2.sendBuffer();                   // transfer internal memory to the display
        delay(100);
        break;
      default:
        bank = 0;
        u8g2.setFontMode(0);                 // enable transparent mode, which is faster
        u8g2.setFont(u8g2_font_04b_03b_tr);  // choose a suitable font
        u8g2.drawStr(100, 5, "Bank 1");      // write something to the internal memory
        u8g2.sendBuffer();                   // transfer internal memory to the display
        delay(100);
        break;
    }
  }
}

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

// Mouse Movement - 240229 J. Alvarez

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
  // read the analog input:
  int reading = analogRead(thisAxis);
  // map the reading from the analog input range to the output range:
  reading = map(reading, 0, 1023, 0, cursorSpeed);
  // if the output reading is outside from the
  // rest position threshold,  use it:
  int distance = reading - center;
  if (abs(distance) < threshold) {
    distance = 0;
  }
  // return the distance for this axis:
  return distance;
}

// Mouse Movement Alternative

void mousemovement() {

  analogX.update();
  analogY.update();

  if (analogX.hasChanged()) {
    horizontal = analogX.getValue();
    if (horizontal > 480 && horizontal < 510) {
      // dead zone - don't move mouse pointer
    } else {
      //horizontal = horizontal - 20;
      mouseX = map(horizontal, 0, 1023, 0, screensizeX);
    }
  }

  if (analogY.hasChanged()) {
    vertical = analogY.getValue();
    if (vertical > 480 && vertical < 510) {
      // dead zone - don't move mouse pointer
    } else {
      mouseY = map(vertical, 0, 1023, 0, screensizeY);
    }
  }

  Mouse.moveTo(mouseX, mouseY);
  delay(20);
}
