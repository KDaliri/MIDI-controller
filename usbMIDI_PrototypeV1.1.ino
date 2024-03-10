#include <Bounce.h>


/*************DECLARATIONS*************/

/*******MACROS*******/
#define channel 1   // MIDI channel assignment
//#define a_pins 10       // number of Analog PINS, HEAD UNIT, with bank option
//#define a_fsPins 2  // number of Analog PINS for Expression Pedal MIDI, with bank option
#define d_pins 6    // number of digital pins per unit (6 on head 6 on footswitch)
#define t_bounce 10   // Debouncing time for switches, in ms
#define on_velocity 127   // velocity for toggle ON
#define off_velocity 0    // veloctiy for toggle OFF
#define messageType 12    // pin for PC_mode state

/*******VARIABLE DECLARATIONS*******/
byte bank, checkState, stateChecked, progPin, progSet, oldMode, oldPin, oldBank; // variables named according to use

/*******ARRAY DECLARATION*******/
byte pinState1[d_pins];   // save state of switches on head unit
byte pinState2[d_pins];   // save state of switches on footswitch unit
byte fsValue[2][d_pins];  // saved CC/PC message per switch on footswitch unit
bool PC_mode[2][d_pins];  // indicates if saved MIDI message is CC or PC on footswitch

//const int analogPin[a_pins] = {A0, A1, A2, A3, A4, A5, A6, A7, A8, A9};  //---Knobs Assignment 240301
//const int fs_analogPin[a_fsPins] = {A12, A13};                      //---Expression Pedal Assignment 240301
const byte digitalPin[d_pins] = {1, 2, 3, 4, 5, 6};    // digital pins for the head unit
const byte fs_digitalPin[d_pins] = {28, 29, 30, 31, 32, 33};   // digital pins for the footswitch unit

// Knobs MIDI CC assignment, per bank
/*const byte CC_knob[2][a_pins] = {
  {21, 22, 23, 24, 25, 26, 27, 28, 29, 30},  //Bank 1
  {31, 32, 33, 34, 35, 36, 37, 38, 39, 40},  //Bank 2
};

// Expression Pedal MIDI CC assignment, per bank
const byte CC_expPedal[2][a_fsPins] = {
  {41, 42},  //Bank 1
  {43, 44},  //Bank 2
};*/

const byte CC_toggle[2][d_pins] = {   // CC assignments for head unit
  {45, 46, 47, 48, 49, 50},   // Bank 1
  {53, 54, 55, 56, 57, 58},   // Bank 2
};

const byte CC_footswitch[2][d_pins] = {   // CC assignments for footswitch unit
  {61, 62, 63, 64, 65, 66},   // Bank 1
  {69, 70, 71, 72, 73, 74},   // Bank 2
};

const byte PC_footswitch[2][d_pins] = {   // PC assignments for footswitch unit
  {0, 1, 2, 3, 4, 5},   // Bank 1
  {6, 7, 8, 9, 10, 11},   // Bank 2
};

/*******BOUNCE LIBRARY DECLARATIONS*******/
Bounce toggle[6] = {    // declare digital pins from the head unit as bounce variables
  Bounce(digitalPin[0], t_bounce),
  Bounce(digitalPin[1], t_bounce),
  Bounce(digitalPin[2], t_bounce),
  Bounce(digitalPin[3], t_bounce),
  Bounce(digitalPin[4], t_bounce),
  Bounce(digitalPin[5], t_bounce),
};

Bounce footswitch[6] = {    // declare digital pins from the footswitch unit as bounce variables
  Bounce(fs_digitalPin[0], t_bounce),
  Bounce(fs_digitalPin[1], t_bounce),
  Bounce(fs_digitalPin[2], t_bounce),
  Bounce(fs_digitalPin[3], t_bounce),
  Bounce(fs_digitalPin[4], t_bounce),
  Bounce(fs_digitalPin[5], t_bounce),
};

Bounce progMode = Bounce(11, t_bounce);   // enter programming mode
Bounce setBank = Bounce(0, t_bounce);    // toggle active bank or program MIDI message to active bank
Bounce setMessage = Bounce(12, t_bounce);    // toggle footswitch unit programming of PC or CC MIDI message. applies if not programming CC from head unit
//Bounce expPedal1 = Bounce(9, t_bounce);   // on/off switch of Expression Pedal 1; Alvarez 240301
//Bounce expPedal2 = Bounce(10, t_bounce);   // on/off switch of Expression Pedal 2. Alvarez 240301


/*************SETUP AND MAIN LOOP*************/

void setup() {
  // put your setup code here, to run once:
  for (progPin = 0; progPin < d_pins; progPin++) {
    pinMode(digitalPin[progPin], INPUT_PULLUP);   // initialize digital pins on the head unit as inputs
    pinMode(fs_digitalPin[progPin], INPUT_PULLUP);   // initialize digital pins on the footswitch unit as inputs
    fsValue[0][progPin] = PC_footswitch[0][progPin];  // initial footswitch unit MIDI messages set to PC for all of bank 1
    fsValue[1][progPin] = PC_footswitch[1][progPin];  // initial footswitch unit MIDI messages set to PC for all of bank 2
    PC_mode[0][progPin] = 1;  // indicate initial footswitch unit MIDI message is PC for all of bank 1
    PC_mode[1][progPin] = 1;  // indicate initial footswitch unit MIDI message is PC for all of bank 2
  }

  pinMode(0, INPUT_PULLUP);   // initialize bank toggle as input
  pinMode(11, INPUT_PULLUP);    // initialize programming mode toggle as input
  pinMode(12, INPUT_PULLUP);    // initialize program CC or PC message toggle as input
  //pinMode(28, INPUT_PULLUP);  // EXPRESSION PEDAL 1 240301 J. Alvarez
 // pinMode(29, INPUT_PULLUP);  // EXPRESSION PEDAL 1 240301 J. Alvarez

  if (digitalRead(0) == true) {   // set MIDI message bank initial state based on bank toggle position
    bank = 0;   // bank 1 active
  }
  else
    bank = 1;   // bank 2 active
}

void loop() {
  // put your main code here, to run repeatedly:
  switches();   // check if a switch on the head unit has been toggled 
  fs_switches();    // check if a switch in the footswitch unit has been toggled

  selectBank();   // check which bank is active

  programmingMode();    // check if programming mode has been toggled entered
  
  while (usbMIDI.read()) {
  }
}


/*************PROGRAMMING SECTION*************/

/*******SELECT FOOTSWITCH*******/
void programmingMode() {
  progMode.update();    // update state of programming mode toggle
  
  if (progMode.fallingEdge()) {   // programming mode toggled, enters programming mode
    while (!progMode.risingEdge()) {    // stay in programming mode until programming mode is toggled again
      progMode.update();    // update state of programming mode toggle
      
      for (progPin = 0; progPin < d_pins; progPin++) {
        setMessage.update();    // update state of program PC/CC message toggle. ensures the state of the pin is tracked correctly when entering the next programming mode loop
        setBank.update();   // update state of bank toggle. ensures the state of the pin is tracked correctly when entering the next programming mode loop
        footswitch[progPin].update();   // update state of switches on the footswitch unit
        
        if (footswitch[progPin].fallingEdge() || footswitch[progPin].risingEdge()) {    // switch on the footswitch unit is toggled. indicates this switch is being programmed
          oldPin = fsValue[bank][progPin];    //save initial value of footswitch unit switch. remedies problem with premature programming if exiting programming mode before finishing programming mode loop
          oldMode = PC_mode[bank][progPin];   // save inital value of footswitch unit MIDI message mode. remedies problem with premature programming if exiting programming mode before finishing programming mode loop
          oldBank = bank;   // saves selected bank when entering programming mode. remedies problem with premature programming if exiting programming mode before finishing programming mode loop
          map2fs();  // toggle a switch. its MIDI message will be mapped to the switch being programmed
        }
        else  {
        toggle[progPin].update();   // update state of switches on the head unit. ensures the state of the pin is tracked correctly when entering next phase of the programming mode loop
        }
      }
    }
  } 
}

/*******MAP MESSAGE*******/
void map2fs() {
  elapsedMillis(10);  // manual "debouncing." next statement reads pin state directly, bypassing bounce library debouncing
  
  while (1) {   // wait for user to select the MIDI message to be programmed
    if (stateChecked == 0) {    //check and save current state of mapable inputs
      for (checkState = 0; checkState < d_pins; checkState++) {
        pinState1[checkState] = digitalRead(digitalPin[checkState]);    // read state from swtiches on the head unit
        pinState2[checkState] = digitalRead(fs_digitalPin[checkState]);   // read state of switches on the footswitch unit
      }
      stateChecked = 1;  // initial state of mapable inputs saved. state will not be checked again
    }
    
    for (checkState = 0; checkState < d_pins; checkState++) {
      selectBank();   // check which bank is active
      
      if (pinState1[checkState] != digitalRead(digitalPin[checkState])) {   // switch from head unit is toggled. program MIDI message of this switch
        fsValue[bank][progPin] = CC_toggle[bank][checkState];   // save CC message of selected switch and active bank to switch being programmed
        PC_mode[bank][progPin] = false;   // indicate saved MIDI message is not PC
        //Serial.println("CC toggle set");
        stateChecked = 0;   //re-check state of mappable inputs next time this function is called             
      }
      
      if (pinState2[checkState] != digitalRead(fs_digitalPin[checkState])) {    // program MIDI message of selected switch from footswitch unit
        if (digitalRead(messageType) == 1) {    // indicates PC message should be programmed
          fsValue[bank][progPin] = PC_footswitch[bank][checkState];   // save PC message of selected switch and active bank to switch being programmed
          PC_mode[bank][progPin] = true;    // indicate saved MIDI message is PC
          //Serial.println("PC fs set");
        }
        if (digitalRead(messageType) == 0) {    // indicates CC message should be programmed
          fsValue[bank][progPin] = CC_footswitch[bank][checkState];   // save CC message of selected switch and active bank to switch being programmed
          PC_mode[bank][progPin] = false;   // indicate saved MIDI message is not PC
          //Serial.println("CC fs set");
        }  
        stateChecked = 0;   //re-check state of mappable inputs next time this function is called               
      }
    }

    progMode.update();    // update state of programming mode toggle
    
    if (progMode.risingEdge()) {    // programming mode toggled
      fsValue[oldBank][progPin] = oldPin;   // enures initial MIDI message is still programmed
      PC_mode[oldBank][progPin] = oldMode;    // ensures initial PC_mode state is not overwritten
      stateChecked = 0;   //re-check state of mappable inputs next time this function is called
      //Serial.println("fs not changed");
      break;    // exits programming mode
    }

    for (checkState = 0; checkState < d_pins; checkState++) {
      toggle[checkState].update();    // update state of switches on the head unit
      footswitch[checkState].update();    // update state of switches on the footswitch unit
    }

    if (footswitch[0].risingEdge() || footswitch[0].fallingEdge() || footswitch[1].risingEdge() || footswitch[1].fallingEdge() || footswitch[2].risingEdge() || footswitch[2].fallingEdge() ||
    footswitch[3].risingEdge() || footswitch[3].fallingEdge() || footswitch[4].risingEdge() || footswitch[4].fallingEdge() || footswitch[5].risingEdge() || footswitch[5].fallingEdge() ||
    toggle[0].risingEdge() || toggle[0].fallingEdge() || toggle[1].risingEdge() || toggle[1].fallingEdge() || toggle[2].risingEdge() || toggle[2].fallingEdge() || toggle[3].risingEdge() ||
    toggle[3].fallingEdge() || toggle[4].risingEdge() || toggle[4].fallingEdge() || toggle[5].risingEdge() || toggle[5].fallingEdge()) {    // a switch has been toggled and its MIDI message mapped
      break;    // exits programming mode
    }
  }
}


/*************DIGITAL SECTION*************/

/*******HEAD UNIT*******/
void switches() {
  for (checkState = 0; checkState < d_pins; checkState++) {
    toggle[checkState].update();    // update state of switches on the head unit
    
    if (toggle[checkState].fallingEdge()) {   // switch on head unit toggled
      usbMIDI.sendControlChange(CC_toggle[bank][checkState], on_velocity, channel);   // send CC message of toggled switch from active bank
    }
    if (toggle[checkState].risingEdge()) {    // switch on head unit toggled
      usbMIDI.sendControlChange(CC_toggle[bank][checkState], off_velocity, channel);    // send CC message of toggled switch from active bank
    }
  }
}

/*******FOOTSWITCH UNIT*******/
void fs_switches() {
  for (checkState = 0; checkState < d_pins; checkState++) {
    footswitch[checkState].update();    // update state of switches on the footswitch unit
    
    if (footswitch[checkState].fallingEdge()) {   // switch on footswitch unit toggled
      if (PC_mode[bank][checkState] == true) {    // indicate saved MIDI message is PC
        usbMIDI.sendProgramChange(fsValue[bank][checkState], channel);  // send PC message of toggled switch from active bank
      }
      if (PC_mode[bank][checkState] == false) {   // indicate saved MIDI message is CC
        usbMIDI.sendControlChange(fsValue[bank][checkState], on_velocity, channel);   // send CC message of toggled switch from active bank
      }
    }
    if (footswitch[checkState].risingEdge()) {    // switch on footswitch unit toggled
      if (PC_mode[bank][checkState] == true) {    // indicate saved MIDI message is PC
        usbMIDI.sendProgramChange(fsValue[bank][checkState], channel);    // send PC message of toggled switch from active bank
      }
      if (PC_mode[bank][checkState] == false) {   // indicate saved MIDI message is CC
        usbMIDI.sendControlChange(fsValue[bank][checkState], off_velocity, channel);    // send CC message of toggled switch from active bank
      }
    }
  }
}

/*******BANK SELECTION********/
void selectBank() {
  setBank.update();   // update state of switches on the footswitch unit
  
  if (setBank.fallingEdge()) {    // bank select toggled
    bank = 1;   // bank 2 active
  }
  if (setBank.risingEdge()) {   // bank select toggled
    bank = 0;   // bank 1 active
  }
}
