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
// Oled Display I2C
#ifdef USING_DISPLAY

#include <SPI.h>
#include <Wire.h>
//#include <Adafruit_GFX.h>  // Include core graphics library for the display
#include <Adafruit_SSD1306.h>  // Include Adafruit_SSD1306 library to drive the display
//#include <Fonts/FreeMonoBold12pt7b.h>  // Add a custom font
//#include <Fonts/FreeMono9pt7b.h>  // Add a custom font
#define OLED_RESET -1
Adafruit_SSD1306 display(128, 64, &Wire, OLED_RESET);  // Create display - size of the display in pixels

#endif

//////////////////////
// Scales
#include "scaleNames.h"
#include "scales.h"
#include "notes.h"

/////////////////////////////////////////////
// BUTTONS
const int N_BUTTONS = 6; //*  total numbers of buttons. Number of buttons in the Arduino + number of buttons on multiplexer 1 + number of buttons on multiplexer 2...
const int N_BUTTONS_ARDUINO = 6; //* number of buttons connected straight to the Arduino (in order)
const int BUTTON_ARDUINO_PIN[N_BUTTONS] = {4, 5, 6, 7, 8, 9}; //* pins of each button connected straight to the Arduino

int buttonCState[N_BUTTONS] = {0};        // stores the button current value
int buttonPState[N_BUTTONS] = {0};        // stores the button previous value

//#define pin13 1 // uncomment if you are using pin 13 (pin with led), or comment the line if it is not
byte pin13index = 12; //* put the index of the pin 13 of the buttonPin[] array if you are using, if not, comment

// debounce
unsigned long lastDebounceTime[N_BUTTONS] = {0};  // the last time the output pin was toggled
unsigned long debounceDelay = 15;    //* the debounce time; increase if the output flickers


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
int potVar = 0; // Difference between the current and previous state of the pot

int potMidiCState[N_POTS] = {0}; // Current state of the midi value
int potMidiPState[N_POTS] = {0}; // Previous state of the midi value

const int TIMEOUT = 300; //* Amount of time the potentiometer will be read after it exceeds the varThreshold
const int varThreshold = 4; //* Threshold for the potentiometer signal variation
boolean potMoving = true; // If the potentiometer is moving
unsigned long PTime[N_POTS] = {0}; // Previously stored time
unsigned long timer[N_POTS] = {0}; // Stores the time that has elapsed since the timer was reset

// one pole filter
// y[n] = A0 * x[n] + B1 * y[n-1];
// A = 0.15 and B = 0.85
float filterA = 0.3;
float filterB = 0.7;

/////////////////////////////////////////////
// THREADS
// This libs create a "fake" thread. This means you can make something happen every x milisseconds
// We can use that to read something in an interval, instead of reading it every single loop
// In this case we'll read the potentiometers in a thread, making the reading of the buttons faster
ThreadController cpu; //thread master, where the other threads will be added
Thread threadPotentiometers; // thread to control the pots
Thread threadChannelMenu; // thread to control the pots

/////////////////////////////////////////////
// DISPLAY
byte display_pos_x = 0;
byte display_pos_y = 0;
byte display_text_size = 2;

/////////////////////////////////////////////
// IR
int IR_min_reading[2] = {130, 130}; // the min reading you want the IR to have
int IR_max_reading[2] = {500, 500};

byte IR_min_val[2] = {0, 0}; // the min val you want the IR to output
byte IR_max_val[2] = {127, 23};

byte IR_val[2] = {0}; // IR actual value
byte IR_Pval[2] = {0}; // IR previous value

float IR_curve = 1; // for the fscale function

boolean note_is_playing = false;

/////////////////////////////////////////////
// MIDI CHANNEL
byte MIDI_CH = 1; //* MIDI channel to be used
byte BUTTON_MIDI_CH = 0; //* MIDI channel to be used
byte NOTE = 36; //* Lowest NOTE to be used - if not using custom NOTE NUMBER
byte CC = 1; //* Lowest MIDI CC to be used - if not using custom CC NUMBER

/////////////////////////////////////////////
// SCALES
byte scaleIndex = 2;
byte octaveIndex = 1;
int octave[5] = { -24, -12, 0, 12, 24};


void setup() {

  Serial.begin(115200);

#ifdef DEBUG
Serial.begin(115200);
Serial.println("Debug mode");
Serial.println();
#endif

  //////////////////////////////////////
  // Buttons
  // Initialize buttons with pull up resistors
  for (int i = 0; i < N_BUTTONS_ARDUINO; i++) {
    pinMode(BUTTON_ARDUINO_PIN[i], INPUT_PULLUP);
  }

  /////////////////////////////////////////////
  // Threads
  // Potentiometers
  threadPotentiometers.setInterval(15); // every how many millisiconds
  threadPotentiometers.onRun(potentiometers); // the function that will be added to the thread
  cpu.add(&threadPotentiometers); // add every thread here


  /////////////////////////////////////////////
  // DISPLAY
#ifdef USING_DISPLAY

  // DISPLAY
  delay(100);  // This delay is needed to let the display to initialize
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // Initialize display with the I2C address of 0x3C
  //display.begin(SSD1306_SWITCHCAPVCC, 0x7A);  // Initialize display with the I2C address of 0x3C
  display.clearDisplay();  // Clear the buffer
  display.setTextColor(WHITE);  // Set color of the text
  display.setRotation(0);  // Set orientation. Goes from 0, 1, 2 or 3
  display.setTextWrap(true);  // By default, long lines of text are set to automatically “wrap” back to the leftmost column.
  // To override this behavior (so text will run off the right side of the display - useful for
  // scrolling marquee effects), use setTextWrap(false). The normal wrapping behavior is restored
  // with setTextWrap(true).
  display.dim(0);  //Set brightness (0 is maximun and 1 is a little dim)

  display.setTextSize(display_text_size);
  //  display.setFont(&FreeMonoBold12pt7b);  // Set a custom font
  //display.setFont(&FreeMono9pt7b);  // Set a custom font
  //display.setFont(&FreeSansBoldOblique9pt7b);  // Set a custom font

  //  display.display();  // Print everything we set previously

  scaleIndex = 2;
  octaveIndex = 1;

  printDisplay(scaleNames[scaleIndex], octave[octaveIndex], MIDI_CH);


#endif

  /////////////////////////////////////////////



}

void loop() {

  //#ifdef ATMEGA32U4
  //
  //  // it will read MIDI incoming messages if using AT32U4
  //  MIDIread();
  //#endif

  //buttons();

  cpu.run(); // for threads

}

/////////////////////////////////////////////
// BUTTONS
void buttons() {

  // read pins from arduino
  for (int i = 0; i < N_BUTTONS_ARDUINO; i++) {
    buttonCState[i] = digitalRead(BUTTON_ARDUINO_PIN[i]);
  }

  for (int i = 0; i < N_BUTTONS; i++) { // Read the buttons connected to the Arduino

    if ((millis() - lastDebounceTime[i]) > debounceDelay) {

      if (buttonPState[i] != buttonCState[i]) {
        lastDebounceTime[i] = millis();

        if (buttonCState[i] == LOW) {
          // DO STUFF

        }
        else {

        }


        buttonPState[i] = buttonCState[i];
      }

    }
  }
}

/////////////////////////////////////////////
// POTENTIOMETERS
void potentiometers() {

  // reads the pins from arduino
  for (int i = 0; i < N_POTS_ARDUINO; i++) {

    // one pole filter
    // y[n] = A0 * x[n] + B1 * y[n-1];
    // A = 0.15 and B = 0.85
    int reading = analogRead(POT_ARDUINO_PIN[i]);
    float filteredVal = filterA * reading + filterB * potPState[i]; // filtered value
    potCState[i] = filteredVal;

    //fscale( float originalMin, float originalMax, float newBegin, float newEnd, float inputValue, float curve) - linearize curve
    potCState[i] = fscale(IR_min_reading[i], IR_max_reading[i], IR_min_reading[i], IR_max_reading[i], potCState[i], IR_curve);

    // sets the value to the desired range
    int val = map(potCState[i], IR_min_reading[i], IR_max_reading[i], IR_min_val[i], IR_max_val[i]);

    // clips IR value to min-max
    //IR_val[i] = clipValue(val, IR_min_val[i], IR_max_val[i]);
    IR_val[i] = val;

    //Debug only
    //  for (int i = 0; i < N_POTS_ARDUINO; i++) {
    //    Serial.print("IR");
    //    Serial.print(i);
    //    Serial.print(": ");
    //    Serial.print(potCState[i]);
    //    Serial.print(" | ");
    //    Serial.print(IR_val[i]);
    //    Serial.print("    ");
    //  }
    //  Serial.println();


    potMidiCState[i] = map(potCState[i], 0, 1023, 0, 127); // Maps the reading of the potCState to a value usable in midi

    potVar = abs(potCState[i] - potPState[i]); // Calculates the absolute value between the difference between the current and previous state of the pot

    if (potVar > varThreshold) { // Opens the gate if the potentiometer variation is greater than the threshold
      PTime[i] = millis(); // Stores the previous time
    }

    timer[i] = millis() - PTime[i]; // Resets the timer 11000 - 11000 = 0ms

    if (timer[i] < TIMEOUT) { // If the timer is less than the maximum allowed time it means that the potentiometer is still moving
      potMoving = true;
    }
    else {
      potMoving = false;
    }


    if (potMoving == true) { // If the potentiometer is still moving, send the change control

      if ((millis() - lastDebounceTime[i]) > debounceDelay) {

        if (IR_val[i] != IR_Pval[i]) {
          if (IR_val[i] >= 0) {
            lastDebounceTime[i] = millis();

#ifdef DEBUG
Serial.print("IR");
Serial.print(i);
Serial.print(": ");
Serial.print(potCState[i]);
Serial.print(" | ");
Serial.print(IR_val[i]);
Serial.println("    ");
#endif

            if (i == 0) { // CC
#ifdef ATMEGA32U4

              controlChange(MIDI_CH, CC + i, IR_val[i]);//  (channel, CC number,  CC value)
              MidiUSB.flush();
#endif

            }

            if (i == 1) { // NOTE
#ifdef ATMEGA32U4

              int a = 2;
              int b = 1;             

              noteOn(MIDI_CH,+ scales[a][6], 127);  // channel, note, velocity
              //              MidiUSB.flush();
              //              noteOn(MIDI_CH, NOTE + scales[scaleIndex][IR_Pval[i]] + octave[octaveIndex], 0);  // shuts down previous note
              //              MidiUSB.flush();

#endif

              note_is_playing = true;
            }
          }
        }
      }

      IR_Pval[i] = IR_val[i];
      potPState[i] = potCState[i]; // Stores the current reading of the potentiometer to compare with the next
    }

    // shuts down the last not when there's nothing in the sensor
    if (i == 1) {

      if (filteredVal < IR_min_reading[1]) {

        if (note_is_playing == true) {

#ifdef ATMEGA32U4

          // shuts down all notes (kill)
          for (int i = 0; i < 127; i++) {
            noteOn(MIDI_CH, i, 0);
            MidiUSB.flush();
          }
#endif

#ifdef DEBUG
Serial.println("KILL");
#endif

          note_is_playing = false;
        }
      }
    }
  }
}




////////////////////////////////////////////
// checks if it's greater than maximum value or less than then the minimum value
int clipValue(int in, int minVal, int maxVal) {

  int out;

  if (in > maxVal) {
    out = maxVal;
  }
  else if (in < minVal) {
    out = minVal;
  }
  else {
    out = in;
  }

  return out;
}


/////////////////////////////////////////////
// if using with ATmega32U4 (micro, pro micro, leonardo...)
#ifdef ATMEGA32U4

// Arduino (pro)micro midi functions MIDIUSB Library
void noteOn(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOn = {0x09, 0x90 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOn);
}

void noteOff(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOff = {0x08, 0x80 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOff);
}

void controlChange(byte channel, byte control, byte value) {
  midiEventPacket_t event = {0x0B, 0xB0 | channel, control, value};
  MidiUSB.sendMIDI(event);
}


void MIDIread() {

  midiEventPacket_t rx = MidiUSB.read();
  switch (rx.header) {
    case 0:
      break; //No pending events

    case 0x9:
      handlennOn(
        rx.byte1 & 0xF,  //channel
        rx.byte2,        //pitch
        rx.byte3         //velocity
      );
      break;

    case 0x8:
      handlennOff(
        rx.byte1 & 0xF,  //channel
        rx.byte2,        //pitch
        rx.byte3         //velocity
      );
      break;
  }

  if (rx.header != 0) {
    //Serial.print("Unhandled MIDI message: ");
    //      Serial.print(rx.byte1 & 0xF, DEC);
    //      Serial.print("-");
    //      Serial.print(rx.byte1, DEC);
    //      Serial.print("-");
    //      Serial.print(rx.byte2, DEC);
    //      Serial.print("-");
    //      Serial.println(rx.byte3, DEC);
  }

}

void handleControlChange(byte channel, byte number, byte value)
{

}

void handlennOn(byte channel, byte number, byte value) {
}


void handlennOff(byte channel, byte number, byte value) {
}


#endif


#ifdef USING_DISPLAY

/////////////////////////////////////////////
// prints the MIDI channel in the display
void printDisplay(char* scaleName_, int octave_, byte channel_) {

  //  display.setCursor(display_pos_x, display_pos_y);  // posição onde vai começar a ser mostrado na tela (x,y)
  display.clearDisplay();  // Clear the display so we can refresh

  display.setCursor(0, 0);
  display.print("Scl: ");  // Imprime no display
  display.println(scaleName_);

  display.setCursor(0, 32);
  display.print("Oct: ");  // Imprime no display
  display.println(octave_);

  display.print("Ch: ");  // Imprime no display
  display.println(channel_);

  display.display();
}
#endif

//////////////////////////////////////////////////////////////////////////////
float fscale( float originalMin, float originalMax, float newBegin, float
              newEnd, float inputValue, float curve) {

  float OriginalRange = 0;
  float NewRange = 0;
  float zeroRefCurVal = 0;
  float normalizedCurVal = 0;
  float rangedValue = 0;
  boolean invFlag = 0;


  // condition curve parameter
  // limit range

  if (curve > 10) curve = 10;
  if (curve < -10) curve = -10;

  curve = (curve * -.1) ; // - invert and scale - this seems more intuitive - postive numbers give more weight to high end on output
  curve = pow(10, curve); // convert linear scale into lograthimic exponent for other pow function

  /*
    Serial.println(curve * 100, DEC);   // multply by 100 to preserve resolution
    Serial.println();
  */

  // Check for out of range inputValues
  if (inputValue < originalMin) {
    inputValue = originalMin;
  }
  if (inputValue > originalMax) {
    inputValue = originalMax;
  }

  // Zero Refference the values
  OriginalRange = originalMax - originalMin;

  if (newEnd > newBegin) {
    NewRange = newEnd - newBegin;
  }
  else
  {
    NewRange = newBegin - newEnd;
    invFlag = 1;
  }

  zeroRefCurVal = inputValue - originalMin;
  normalizedCurVal  =  zeroRefCurVal / OriginalRange;   // normalize to 0 - 1 float

  /*
    Serial.print(OriginalRange, DEC);
    Serial.print("   ");
    Serial.print(NewRange, DEC);
    Serial.print("   ");
    Serial.println(zeroRefCurVal, DEC);
    Serial.println();
  */

  // Check for originalMin > originalMax  - the math for all other cases i.e. negative numbers seems to work out fine
  if (originalMin > originalMax ) {
    return 0;
  }

  if (invFlag == 0) {
    rangedValue =  (pow(normalizedCurVal, curve) * NewRange) + newBegin;

  }
  else     // invert the ranges
  {
    rangedValue =  newBegin - (pow(normalizedCurVal, curve) * NewRange);
  }

  return rangedValue;
}
