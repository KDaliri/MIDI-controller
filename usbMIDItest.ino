#include <Bounce.h>                           //Bounce library provides easy setup for debouncing switch/button

int pot, LEDoff;
int preset=1;
int prevpot=-1;

Bounce stomp=Bounce(9, 5);                    //variables for debouncing. Indicates pin and debounce time in ms
Bounce toggle=Bounce(23, 7);

void setup() {
  // put your setup code here, to run once:
pinMode(9,INPUT_PULLUP);
pinMode(23,INPUT_PULLUP);
pinMode(0,OUTPUT);
pinMode(1,OUTPUT);
pinMode(2,OUTPUT);
pinMode(3,OUTPUT);
digitalWrite(0,HIGH);                           //for LED tracking of ProgramChange message. initial ProgramChange value is 0
digitalWrite(1,LOW);                            //LED for PC1 message
digitalWrite(2,LOW);                            //LED for PC2 message
digitalWrite(3,LOW);                            //LED for PC3 message
}

void loop() {
  // put your main code here, to run repeatedly:
pot=analogRead(A12)/8;                          //convert pot value to MIDI value. MIDI=0-127. pot=0-1023
toggle.update();                                //upadate button status
stomp.update();

switches();                                     //send a MIDI ControlChange message when a switch is toggled
recall();                                       //send a MIDI ProgramChange message when a button is pressed
knobs();                                        //send MIDI ControlChange messages as a knob turns

while(usbMIDI.read()){                          //required to prevent issues according to teensy website
}
}

void switches(){
  if(toggle.fallingEdge()){
    usbMIDI.sendControlChange(65,64,1);
  }
  if(toggle.risingEdge()){
    usbMIDI.sendControlChange(65,63,1);
  }
}

void recall(){
  if(stomp.fallingEdge()){
    usbMIDI.sendProgramChange(preset,1);        //ProgramChange typically used to recall saved presets. Preset value corresponds to ProgramChange value
    digitalWrite(preset,!digitalRead(preset));  //For LED tracking of ProgramChange value. Turns LED that corresponds to the sent ProgramChange value on
    LEDoff=preset-1;                            //Saves pin value of previous ProgramChange message
    if(preset==0){                              //fix LEDoff value after resetting ProgramChange value to initial state
      LEDoff=3;
    }
    digitalWrite(LEDoff,!digitalRead(LEDoff));  //turns LED that corresponds to previous ProgramChange value off
    ++preset;                                   //Cycle to one of 4 ProgramChange values each time the function is called
    if(preset==4){                              //Reset ProgramChange value to initial state
      preset=0;
    }
  }
}

void knobs(){
  if(pot>prevpot+1||pot<prevpot-1){             //sensitivity is -1 to +1
    pot=analogRead(A12)/8;                      //recheck value for accuracy
    usbMIDI.sendControlChange(52,pot,1);
    prevpot=pot;
  }
}
