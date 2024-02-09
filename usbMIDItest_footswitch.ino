#include <Bounce.h>                           //Bounce library provides easy setup for debouncing switch/button
#include <MIDI.h>                             
/*Since usbMIDI.read() only reads MIDI through the usb port,
  serial MIDI.read() is used to interpret MIDI messages being sent by usbMIDI.
  Instead of reading the actual message, a duplicate message is sent through a serial MIDI output and is shorted to the corresponding serial MIDI input.
  This allows MIDI messages sent by usbMIDI to be interpreted for footswitch programming*/

MIDI_CREATE_INSTANCE(HardwareSerial, Serial2, MIDI);

int pot, LEDoff, checkstate;
int preset=1;
int prevpot=-1;
int fs1=0;
int value=0;
int recheck=1;
int fsset=0;
int digiIN[2]={22,23};
bool pinstate[2];

Bounce stomp=Bounce(9, 5);                    //variables for debouncing. Indicates pin and debounce time in ms
Bounce toggle=Bounce(23, 7);
Bounce stomp2=Bounce(5, 5);
Bounce toggle2=Bounce(22, 7);                    
Bounce program=Bounce(27, 7);

void setup() {
  // put your setup code here, to run once:
pinMode(5,INPUT_PULLUP);
pinMode(9,INPUT_PULLUP);
pinMode(22,INPUT_PULLUP);
pinMode(23,INPUT_PULLUP);
pinMode(27,INPUT_PULLUP);
pinMode(0,OUTPUT);
pinMode(1,OUTPUT);
pinMode(2,OUTPUT);
pinMode(3,OUTPUT);
pinMode(24,OUTPUT);
pinMode(26,OUTPUT);
digitalWrite(0,HIGH);                           //for LED tracking of ProgramChange message. initial ProgramChange value is 0
digitalWrite(1,LOW);                            //LED for PC1 message
digitalWrite(2,LOW);                            //LED for PC2 message
digitalWrite(3,LOW);                            //LED for PC3 message
digitalWrite(24,LOW);                           //Footswitch programming mode LED. LED on indicates programming mode active
digitalWrite(26,LOW);                           //Footswitch1 LED. LED on indicates footswitch is selected
MIDI.begin(1);                                  //initialise serial MIDI
}

void loop() {
  // put your main code here, to run repeatedly:
pot=analogRead(A0)/8;                          //convert pot value to MIDI value. MIDI=0-127. pot=0-1023
toggle.update();                               //update switch/button status
stomp.update();
toggle2.update();
stomp2.update();
program.update();

programming();                                  //footswitch message programming
switches();                                     //send a MIDI ControlChange message when a switch is toggled
recall();                                       //send a MIDI ProgramChange message when a button is pressed
knobs();                                        //send MIDI ControlChange messages as a knob turns
footswitch();                                   //send MIDI message from footswitch input

while(usbMIDI.read()){
}
}

void programming(){
  if(program.fallingEdge()){                    //Toggle switch enters footswitch programming mode
    digitalWrite(24,HIGH);                      //LED indicates controller is in programming mode
    while(!program.risingEdge()){               //Stay in programming mode until switch is toggled again
      digitalWrite(26,LOW);                     //all active footswitch LEDs turn off
      program.update();
      stomp2.update();
      if(stomp2.fallingEdge()||stomp2.risingEdge()){//program footswitch1
        digitalWrite(26,HIGH);                   //LED indicates footswitch1 is being programmed       
        while(!program.risingEdge()&&(fsset==0)){//wait for user to select the MIDI message to be programmed from main unit mapping. Can still toggle out of programming mode
          program.update();
          map2fs();                             //determine which MIDI message to map to footswitch
        }
        fs1=value;                              //map control/program change to footswitch1
        fsset=0;                                //condition to enter loop that calls map2fs()
      }
    }
  }
digitalWrite(24,LOW);                           //LED indicates controller is exiting programming mode
}

void map2fs(){
  if(recheck==1){
    for(checkstate=0;checkstate<2;++checkstate){           //check and save current state of mapable inputs
      pinstate[checkstate]=digitalRead(digiIN[checkstate]);
    }
    recheck=0;                                             //prevents constently updating saved state while waiting for user to map message
  }
  if(pinstate[0]!=digitalRead(digiIN[0])){                 //execute if curernt state of input differs from saved state
    MIDI.sendControlChange(66,64,1);                       //sends serial MIDI message that corresponds to usbMIDI message from the same input
    delayMicroseconds(10);                                 //works without but helps with consistent detection? experimental
    while(!MIDI.read()){
    //wait for serial MIDI message to be recieved before trying to fetch data. Calling MIDI.read() directly rarely fetches data for some reason
    }
    value=MIDI.getData1();                                 //stores control/program change value
    fsset=1;                                               //condition to exit loop that calls this function
    recheck=1;                                             //re-enable checking and saving state of mappable inputs next time this function is called
  }
  if(pinstate[1]!=digitalRead(digiIN[1])){
    MIDI.sendControlChange(65,64,1);
    delayMicroseconds(10);
    while(!MIDI.read()){
    }
    value=MIDI.getData1();
    fsset=1;
    recheck=1;
  }
}

void switches(){
  if(toggle.fallingEdge()){
    usbMIDI.sendControlChange(65,64,1);
  }
  if(toggle.risingEdge()){
    usbMIDI.sendControlChange(65,63,1);
  }
   if(toggle2.fallingEdge()){
    usbMIDI.sendControlChange(66,64,1);
  }
  if(toggle2.risingEdge()){
    usbMIDI.sendControlChange(66,63,1);      
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
    pot=analogRead(A0)/8;                       //read value again for accuracy
    usbMIDI.sendControlChange(52,pot,1);
    prevpot=pot;
  }
}

void footswitch(){
  if(stomp2.fallingEdge()){
    usbMIDI.sendControlChange(fs1,64,1);
    digitalWrite(26,HIGH);
  }
  if(stomp2.risingEdge()){
    usbMIDI.sendControlChange(fs1,63,1);
    digitalWrite(26,LOW);
  }
}