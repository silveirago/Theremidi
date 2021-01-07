#ifdef USING_DISPLAY

/////////////////////////////////////////////
// prints the MIDI channel in the display
void printDisplay(char* scaleName, int root, int octave, byte channel, char* PB) {

  //  display.setCursor(display_pos_x, display_pos_y);  // posição onde vai começar a ser mostrado na tela (x,y)
  display.clearDisplay();  // Clear the display so we can refresh

  display.setCursor(0, 0);
  display.print("Scl:");  // Imprime no display
  display.println(scaleName);

  display.setCursor(0, 16);
  display.print("Root: ");  // Imprime no display
  display.println(noteNames[root]);

  display.setCursor(0, 32);
  display.print("Oct: ");  // Imprime no display
  display.println(octave);

  display.print("Ch: ");  // Imprime no display
  display.print(channel + 1);

  display.print(" |");
  display.print(PB);

  display.display();
}

#endif
