void setup() {

#ifdef DEBUG
  Serial.begin(115200);
  delay(2000);
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
  threadIR0.setInterval(2); // every how many millisiconds
  threadIR0.onRun(IR0); // the function that will be added to the thread
  cpu.add(&threadIR0); // add every thread here

  threadIR1.setInterval(20); // every how many millisiconds
  threadIR1.onRun(IR1); // the function that will be added to the thread
  cpu.add(&threadIR1); // add every thread here

  MIDI_CH = 0;


  /////////////////////////////////////////////
  // DISPLAY
#ifdef USING_DISPLAY

  /////////////////////////////////////////////
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
  // Print variable with left alignment:
  //  display.setCursor(display_pos_x, display_pos_y);  // posição onde vai começar a ser mostrado na tela (x,y)
  //  //display.println(Variable1);  // Text or value to print
  //  display.print(MIDI_CH);  // Text or value to print
  //  display.display();  // Print everything we set previously

  //void printDisplay(char* scaleName, int root, int octave, byte channel) {
  printDisplay(scaleNames[scaleIndex], NOTE % 12, octave[octaveIndex], MIDI_CH, pitchBendNames[0]);

#endif

  /////////////////////////////////////////////



}
