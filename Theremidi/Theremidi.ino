/*
  Made by Gustavo Silveira, 2019.
  - This Sketch reads the Arduino's digital and analog ports and send midi notes and midi control change
  - It can use many multiplexers

  http://www.musiconerd.com
  http://www.youtube.com/musiconerd
  http://facebook.com/musiconerdmusiconerd
  http://instagram.com/musiconerd/
  http://www.gustavosilveira.net
  gustavosilveira@musiconerd.com

  If you are using for anything that's not for personal use don't forget to give credit.

  PS: Just change the value that has a comment like " //* "

*/

/////////////////////////////////////////////
// Choosing your board
// Define your board, choose:
// "ATMEGA328" if using ATmega328 - Uno, Mega, Nano...
// "ATMEGA32U4" if using with ATmega32U4 - Micro, Pro Micro, Leonardo...
// "TEENSY" if using a Teensy board
// "DEBUG" if you just want to debug the code in the serial monitor
// you don't need to comment or uncomment any MIDI library below after you define your board

#define ATMEGA32U4 1//* put here the uC you are using, like in the lines above followed by "1", like "ATMEGA328 1", "DEBUG 1", etc.


/////////////////////////////////////////////
// Are you using an I2C Oled Display?
#define USING_DISPLAY 1 //* comment if not using an I2C Oled Display.

/////////////////////////////////////////////
// LIBRARIES
// -- Defines the MIDI library -- //


// if using with ATmega328 - Uno, Mega, Nano...
#ifdef ATMEGA328
#include <MIDI.h>
//MIDI_CREATE_DEFAULT_INSTANCE();

// if using with ATmega32U4 - Micro, Pro Micro, Leonardo...
#elif ATMEGA32U4
#include "MIDIUSB.h"

#endif
// ---- //

//////////////////////
// Threads
#include <Thread.h> // Threads library >> https://github.com/ivanseidel/ArduinoThread
#include <ThreadController.h> // Same as above

//////////////////////
// Average
#include <Average.h>


//////////////////////
// Oled Display I2C
#ifdef USING_DISPLAY
//#include <Adafruit_GFX.h>  // Include core graphics library for the display
#include <Adafruit_SSD1306.h>  // Include Adafruit_SSD1306 library to drive the display
//#include <Fonts/FreeMonoBold12pt7b.h>  // Add a custom font
//#include <Fonts/FreeMono9pt7b.h>  // Add a custom font
Adafruit_SSD1306 display(128, 64);  // Create display - size of the display in pixels
#endif

//////////////////////
// scaleNotes
//#include "scaleNames.h"
//#include "scales.h"
//#include "notes.h"

/////////////////////////////////////////////
// BUTTONS
const int N_BUTTONS = 7; //*  total numbers of buttons. Number of buttons in the Arduino + number of buttons on multiplexer 1 + number of buttons on multiplexer 2...
const int N_BUTTONS_ARDUINO = 7; //* number of buttons connected straight to the Arduino (in order)
const int BUTTON_ARDUINO_PIN[N_BUTTONS] = {4, 5, 6, 7, 8, 9, 10}; //* pins of each button connected straight to the Arduino

int buttonCState[N_BUTTONS] = {0};        // stores the button current value
int buttonPState[N_BUTTONS] = {0};        // stores the button previous value

//#define pin13 1 // uncomment if you are using pin 13 (pin with led), or comment the line if it is not
byte pin13index = 12; //* put the index of the pin 13 of the buttonPin[] array if you are using, if not, comment

// debounce
unsigned long lastDebounceTime[N_BUTTONS] = {0};  // the last time the output pin was toggled
unsigned long debounceDelay = 20;    //* the debounce time; increase if the output flickers


/////////////////////////////////////////////
// POTENTIOMETERS
const int N_POTS = 2; //* total numbers of pots (slide & rotary). Number of pots in the Arduino + number of pots on multiplexer 1 + number of pots on multiplexer 2...
const int N_POTS_ARDUINO = 2; //* number of pots connected straight to the Arduino
const int POT_ARDUINO_PIN[N_POTS_ARDUINO] = {A0, A1}; //* pins of each pot connected straight to the Arduino

//#define USING_CUSTOM_CC_N 1 //* comment if not using CUSTOM CC NUMBERS, uncomment if using it.
#ifdef USING_CUSTOM_CC_N
int POT_CC_N[N_POTS] = {1, 2, 3, 4, 5, 6, 7, 8}; // Add the CC NUMBER of each pot you want
#endif


int potCState[N_POTS] = {0}; // Current state of the pot
int potPState[N_POTS] = {0}; // Previous state of the pot
int potVar[N_POTS] = {0}; // Difference between the current and previous state of the pot

int reading[N_POTS] = {0};
int filteredVal[N_POTS] = {0};
int scaledVal[N_POTS] = {0};
int rangedVal[N_POTS] = {0};

int potMidiCState[N_POTS] = {0}; // Current state of the midi value
int potMidiPState[N_POTS] = {0}; // Previous state of the midi value

const int TIMEOUT[3] = {300, 300}; //* Amount of time the potentiometer will be read after it exceeds the varThreshold[i]
const int varThreshold[3] = {3, 3}; //* Threshold for the potentiometer signal variation
boolean potMoving[3] = {true}; // If the potentiometer is moving
unsigned long PTime[3] = {0}; // Previously stored time
unsigned long timer[3] = {0}; // Stores the time that has elapsed since the timer was reset

// one pole filter
// y[n] = A0 * x[n] + B1 * y[n-1];
// A = 0.15 and B = 0.85
float filterA = 0.15;
float filterB = 0.85;

/////////////////////////////////////////////
// THREADS
// This libs create a "fake" thread. This means you can make something happen every x milisseconds
// We can use that to read something in an interval, instead of reading it every single loop
// In this case we'll read the potentiometers in a thread, making the reading of the buttons faster
ThreadController cpu; //thread master, where the other threads will be added
Thread threadIR0; // thread to control the pots
Thread threadIR1;

/////////////////////////////////////////////
// DISPLAY
byte display_pos_x = 0;
byte display_pos_y = 0;
byte display_text_size = 2;

/////////////////////////////////////////////
// IR
int IR_range[2] = {90, 530}; // the min reading you want the IR to have
//int IR_max_reading[2] = {300, 300};

int IR_min_val[2] = {0, 0}; // the min val you want the IR to output
int IR_max_val[2] = {127, 13};

byte IR_val[2] = {0}; // IR actual value
byte IR_Pval[2] = {0}; // IR previous value

float IR_curve = 1; // for the fscale function

boolean note_is_playing = false;

/////////////////////////////////////////////
// Average
byte averageSize[2] = {10, 15};

Average<float> ave[2] = {
  Average<float> (averageSize[0]),
  Average<float> (averageSize[1])
};


/////////////////////////////////////////////
// MIDI STUFF
int MIDI_CH = 0; //* MIDI channel to be used
byte NOTE = 48; //* Lowest NOTE to be used - The root
byte CC = 7; //* cc to be used in the left sensor
byte sustainCC = 64; //* cc for the sustain pedal
boolean rootMenuIsOn = false;

boolean pitchBendIsOn = false;
boolean pitchBendMenuIsOn = false;
boolean pitchBendIsPlaying = false;

int pitchBendVal = 0;
int pitchBendPVal = 0;

/////////////////////////////////////////////
// scaleNotes
byte scaleIndex = 0;
int octaveIndex = 2;
int octave[5] = { -24, -12, 0, 12, 24};

const byte SCALE_NUM = 4; // number of scales
const byte NOTE_NUM = 16; // number of notes in the scales


int scaleNotes[SCALE_NUM][NOTE_NUM] {

  { -1, 0, 2, 4, 5, 7, 9, 11, 12, 14, 16, 17, 19, 21, 23, 24}, // 0: ionian
  { -1, 0, 2, 3, 5, 7, 8, 11, 12, 14, 15, 17, 19, 20, 23, 24}, // 4: min harm
  { -2, 0, 3, 5, 7, 10, 12, 15, 17, 19, 22, 24, 27, 29, 31, 34}, // 5: minor penta
  { -2, 0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28}, // 7: whltone
  //  { -2, 0, 2, 3, 5, 7, 8, 10, 12, 14, 15, 17, 19, 20, 22, 24}, // 4: aeolian
  //  { -4, 0, 2, 4, 7, 9, 12, 14, 16, 19, 21, 24, 26, 28, 31, 33}, // 5: major pentatonic
  //  { -2, 0, 3, 5, 7, 10, 12, 15, 17, 19, 22, 24, 27, 29, 31, 34}, // 6: minor pentatonic
  //  { -2, 0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28}, // 7: wholetone

};

char* scaleNames[] = {"Jonian", "MinHar", "MinPen", "WhlTon"};

char* noteNames[] = {
  "C", "C#", "D", "D#", "E", "F", "F#",  "G", "G#", "A", "A#", "B"
};

char* pitchBendNames[] = {"Scl", "Pb"};

byte noteOut = 0;
