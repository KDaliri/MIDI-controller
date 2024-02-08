
/* Use arrays to manage lists of knobs/pots and pushbuttons.

   By Leif Oddson
   https://forum.pjrc.com/threads/45376

   Teensy 4.1

   Jan 21, 2024 - Version 1 - J. Alvarez
   Jan 31, 2024 - Version 2 - J. Alvarez - Added SSD1306 OLED Display Capabilities and ENTER keyboard stroke
*/

//************LIBRARIES USED**************
// include the ResponsiveAnalogRead library for analog smoothing
#include <ResponsiveAnalogRead.h>
// include the Bounce library for 'de-bouncing' switches -- removing electrical chatter as contacts settle
#include <Bounce.h>
//usbMIDI.h library is added automatically when code is compiled as a MIDI device
#include <Keyboard.h>
//Keyboard.h library is added

#include <U8g2lib.h> //Added 240131
//---Added 240131
#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif
//---

U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, /* clock=*/24, /* data=*/25, /* reset=*/U8X8_PIN_NONE);  // All Boards without Reset of the Display, Added 240131

// ******CONSTANT VALUES********
// customize code behaviour here!
const int channel = 1;       // MIDI channel
const int A_PINS = 6;        // number of Analog PINS
const int D_PINS = 6;        // number of Digital PINS FOR MIDI
const int ON_VELOCITY = 99;  // note-one velocity sent from buttons (should be 65 to  127)
int tabs = 2;

// define the pins you want to use and the CC ID numbers on which to send them..
const int ANALOG_PINS[A_PINS] = { A0, A1, A2, A3, A4, A5 };
const int CCID[A_PINS] = { 21, 22, 23, 24, 25, 26 };

// define the pins and notes for digital events
const int DIGITAL_PINS[D_PINS] = { 0, 1, 2, 3, 4, 5 }; //---Added Digital pins 3 to 5, 240131
const int note[D_PINS] = { 60, 61, 62, 63, 64, 65 }; //---Added CC notes from 63 to 65, 240131
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

Bounce button8 = Bounce(8, 5);    // keybaord
Bounce button9 = Bounce(9, 5);    // keybaord
Bounce button10 = Bounce(10, 5);  // keybaord
Bounce button11 = Bounce(11, 5);  // keybaord
Bounce button12 = Bounce(12, 5);  // keybaord

//************SETUP**************
void setup() {
  // loop to configure input pins and internal pullup resisters for digital section
  for (int i = 0; i < D_PINS; i++) {
    pinMode(DIGITAL_PINS[i], INPUT_PULLUP);
  }
  pinMode(8, INPUT_PULLUP);   // keyboard stroke ENTER Added 240131
  pinMode(9, INPUT_PULLUP);   // keyboard stroke UP
  pinMode(10, INPUT_PULLUP);  // keyboard stroke DOWN
  pinMode(11, INPUT_PULLUP);  // keyboard stroke ALT+TAB
  pinMode(12, INPUT_PULLUP);  // keyboard stroke RELEASE MODIFIER

  u8g2.begin(); //Added 240131
  u8g2.clearBuffer();  // clear the internal memory, Added 240131

  u8g2.setFont(u8g2_font_inb30_mr);  // set the target font to calculate the pixel width
  width = u8g2.getUTF8Width(text);   // calculate the pixel width of the text
  /*
  u8g2.setFontMode(0);                 // enable transparent mode, which is faster
  u8g2.setFont(u8g2_font_ncenB08_tr);  // choose a suitable font
  u8g2.drawStr(0, 10, "Joseph");       // write something to the internal memory
  u8g2.sendBuffer();                   // transfer internal memory to the display
  delay(1000);
*/
}

//************LOOP**************
void loop() {

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
  
  getAnalogData();
  getDigitalData();
  
  while (usbMIDI.read()) {
    // controllers must call .read() to keep the queue clear even if they are not responding to MIDI
  }
  
  button8.update(); //------Added 240131
  button9.update();
  button10.update();
  button11.update();
  button12.update();
  
  //------Added 240131
  if (button8.fallingEdge()) {
    Keyboard.press(KEY_ENTER);
    Keyboard.release(KEY_ENTER);
    //u8g2.begin();
    u8g2.clearBuffer();                  // clear the internal memory
    u8g2.setFont(u8g2_font_ncenB08_tr);  // choose a suitable font
    u8g2.drawUTF8(28, 31, "ENTER");         // write something to the internal memory
    u8g2.sendBuffer();                   // transfer internal memory to the display
    delay(1000);
  }
  //------Added 240131
  
  if (button9.fallingEdge()) {
    Keyboard.press(KEY_UP);
    Keyboard.release(KEY_UP);
    //u8g2.begin();
    u8g2.clearBuffer();                  // clear the internal memory
    u8g2.setFont(u8g2_font_ncenB08_tr);  // choose a suitable font
    u8g2.drawUTF8(31, 31, "UP");         // write something to the internal memory
    u8g2.sendBuffer();                   // transfer internal memory to the display
    delay(1000);
  }

  if (button10.fallingEdge()) {
    Keyboard.press(KEY_DOWN);
    Keyboard.release(KEY_DOWN);
    u8g2.clearBuffer();                  // clear the internal memory
    u8g2.setFont(u8g2_font_ncenB08_tr);  // choose a suitable font
    u8g2.drawUTF8(30, 31, "DOWN");         // write something to the internal memory
    u8g2.sendBuffer();                   // transfer internal memory to the display
    delay(1000);
  }

  if (button11.fallingEdge()) {
    Keyboard.set_modifier(0);
    Keyboard.send_now();
    //u8g2.begin();
    u8g2.clearBuffer();                          // clear the internal memory
    u8g2.setFont(u8g2_font_ncenB08_tr);          // choose a suitable font
    u8g2.drawUTF8(0, 21, "ALt-Tab - Released");  // write something to the internal memory
    u8g2.sendBuffer();                           // transfer internal memory to the display
    delay(1000);
  }

  if (button12.fallingEdge()) {
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
