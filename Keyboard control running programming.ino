#include<stdio.h>
#include<conio.h>
#include <Keyboard.h>
#include <Bounce.h>
#include <Keyboard.h>


Bounce digital[] = {
  Bounce(DIGITAL_PINS[0], BOUNCE_TIME),
  Bounce(DIGITAL_PINS[1], BOUNCE_TIME),
  Bounce(DIGITAL_PINS[2], BOUNCE_TIME),


Bounce SHIFT_KEY = Bounce(0, 5);    
Bounce ALTtTAB_KEY = Bounce(1, 5);  
Bounce SPACE_KEY = Bounce(2, 5); 


//************SETUP**************
void setup() {
  
  for (int i = 0; i < D_PINS; i++) {
    pinMode(DIGITAL_PINS[i], INPUT_PULLUP);
  }

  pinMode(0, INPUT_PULLUP);  
  pinMode(1, INPUT_PULLUP);  
  pinMode(2, INPUT_PULLUP);  
  

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
void keystrokes() 
{
 
  SHIFT_KEY.update();
  ALTtTAB_KEY.update();
  SPACE.update();
  //------Added 240131
  if (SPACE_KEY.fallingEdge()) {
    Keyboard.press(KEY_SPACE);
    Keyboard.release(KEY_SPACE;
    //u8g2.begin();
    u8g2.clearBuffer();                  
    u8g2.setFont(u8g2_font_ncenB08_tr);  
    u8g2.drawUTF8(25, 27, "SPACE");      
    u8g2.sendBuffer();                   
    delay(100);
  }


  if (ALTtTAB_KEY.fallingEdge()) {
    Keyboard.set_modifier(MODIFIERKEY_ALT);
    Keyboard.send_now();
    u8g2.clearBuffer();                          
    u8g2.setFont(u8g2_font_ncenB08_tr);          
    u8g2.drawUTF8(0, 21, "ALt-Tab - Released");  
    u8g2.sendBuffer();                           
    tab();
    delay(250);
  }
}





}
