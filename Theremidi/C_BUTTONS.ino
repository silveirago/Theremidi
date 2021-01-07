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

          //#ifdef DEBUG
          //Serial.print(i);
          //Serial.print(" ");
          //Serial.println(buttonCState[i]);
          //#endif

          // if midi channel buttons are pressed at the same time they act as a shift button
          // then you can use the octave buttons to change the NOTE (root)
          while ((buttonCState[2] == LOW) && (buttonCState[5] == LOW)) {
            if (rootMenuIsOn == false) {
              rootMenuIsOn = true;
              //Serial.println("rootMenu Is True");
            }
            rootMenu();
          }

          // If both scale buttons are pressed you change the mode of the right sensor
          // between notes and pitch bend

          if ((buttonCState[0] == LOW) && (buttonCState[3] == LOW)) {
            pitchBendIsOn = !pitchBendIsOn;

            if (pitchBendIsOn == true) {
              threadIR1.setInterval(2); // every how many millisiconds
            } else {
              threadIR1.setInterval(20); // every how many millisiconds
            }

            printDisplay(scaleNames[scaleIndex], NOTE % 12, octave[octaveIndex], MIDI_CH, pitchBendNames[pitchBendIsOn]);
            pitchBendMenuIsOn = true;

#ifdef DEBUG
            Serial.print("Pitch Bend menu is ");
            Serial.println(pitchBendMenuIsOn);
            Serial.print("Pitch bend: ");
            Serial.println(pitchBendIsOn);
#endif

          }


          // printDisplay(scaleNames[scaleIndex], NOTE % 12, octave[octaveIndex], MIDI_CH, pitchBendNames[pitchBendIsOn]);

          // sustain pedal
          if (i == 6) {

#ifdef ATMEGA32U4

            controlChange(MIDI_CH, sustainCC, 127);//  (channel, CC number,  CC value)
            MidiUSB.flush();
#endif

#ifdef DEBUG
            Serial.println("Sustain on");
#endif

          }
        }

        else { // if buttons are not pressed

          if ((rootMenuIsOn == false) && (pitchBendMenuIsOn == false)) { // if menus are not on

            switch (i) {
              case 0:
                scaleIndex--;
                scaleIndex %= 4;
                printDisplay(scaleNames[scaleIndex], NOTE % 12, octave[octaveIndex], MIDI_CH, pitchBendNames[pitchBendIsOn]);

#ifdef DEBUG
                Serial.print("Scale: ");
                Serial.println(scaleNames[scaleIndex]);
#endif

                break;
              case 3:
                scaleIndex++;
                scaleIndex %= 4;
                printDisplay(scaleNames[scaleIndex], NOTE % 12, octave[octaveIndex], MIDI_CH, pitchBendNames[pitchBendIsOn]);

#ifdef DEBUG
                Serial.print("Scale: ");
                Serial.println(scaleNames[scaleIndex]);
#endif

                break;
              case 1:
                octaveIndex--;
                if (octaveIndex < 1) octaveIndex = 0;
                printDisplay(scaleNames[scaleIndex], NOTE % 12, octave[octaveIndex], MIDI_CH, pitchBendNames[pitchBendIsOn]);

#ifdef DEBUG
                Serial.print("Octave: ");
                Serial.println(octave[octaveIndex]);
                //Serial.println(octaveIndex);
#endif

                break;
              case 4:
                octaveIndex++;
                if (octaveIndex > 4) octaveIndex = 4;
                printDisplay(scaleNames[scaleIndex], NOTE % 12, octave[octaveIndex], MIDI_CH, pitchBendNames[pitchBendIsOn]);

#ifdef DEBUG
                Serial.print("Octave: ");
                Serial.println(octave[octaveIndex]);
                //Serial.println(octaveIndex);
#endif

                break;
              case 2:
                MIDI_CH--;
                if (MIDI_CH < 0) MIDI_CH = 0;
                printDisplay(scaleNames[scaleIndex], NOTE % 12, octave[octaveIndex], MIDI_CH, pitchBendNames[pitchBendIsOn]);

#ifdef DEBUG
                Serial.print("MIDI Channel: ");
                Serial.println(MIDI_CH);
#endif

                break;
              case 5:
                MIDI_CH++;
                if (MIDI_CH > 15) MIDI_CH = 15;
                printDisplay(scaleNames[scaleIndex], NOTE % 12, octave[octaveIndex], MIDI_CH, pitchBendNames[pitchBendIsOn]);

#ifdef DEBUG
                Serial.print("MIDI Channel: ");
                Serial.println(MIDI_CH);
#endif

                break;
            }
          }

          if (i == 6) { // sustain pedal off

#ifdef ATMEGA32U4

            controlChange(MIDI_CH, sustainCC, 0);//  (channel, CC number,  CC value)
            MidiUSB.flush();
#endif

#ifdef DEBUG
            Serial.println("Sustain off");
#endif

          }

          //#ifdef DEBUG
          //Serial.print(i);
          //Serial.print(" ");
          //Serial.println(buttonCState[i]);
          //#endif
        }

        buttonPState[i] = buttonCState[i];
      }

    }
  }

  // turns off the root menu when both octave buttons are off
  if (rootMenuIsOn == true) {
    if ((buttonCState[2] == HIGH) && (buttonCState[5] == HIGH)) {
      rootMenuIsOn = false;

#ifdef DEBUG
      Serial.println("rootMenu Is Off");
#endif

    }
  }

  // turns off the pitch bend menu when both scale buttons are off
  if (pitchBendMenuIsOn == true) {
    if ((buttonCState[0] == HIGH) && (buttonCState[3] == HIGH)) {
      pitchBendMenuIsOn = false;

#ifdef DEBUG
      Serial.print("Pitch Bend menu is ");
      Serial.println(pitchBendMenuIsOn);
#endif

    }
  }
}

/////////////////////////////////////////////
// create a menu to change the root note pressing
//both ocatve buttons at the same time

void rootMenu() {

  int index[2] = {1, 4};

  // read pins from arduino
  for (int i = 0; i < N_BUTTONS_ARDUINO; i++) {
    buttonCState[i] = digitalRead(BUTTON_ARDUINO_PIN[i]);
  }

  for (int i = 0; i < 2; i++) {
    //    buttonCState[index[i]] = digitalRead(BUTTON_ARDUINO_PIN[index[i]]);

    if ((millis() - lastDebounceTime[index[i]]) > debounceDelay) {

      if (buttonPState[index[i]] != buttonCState[index[i]]) {
        lastDebounceTime[index[i]] = millis();

        if (buttonCState[index[i]] == LOW) {
          // DO STUFF

          switch (i) {
            case 0:
              NOTE--;
              if (NOTE < 36) NOTE = 36;
              printDisplay(scaleNames[scaleIndex], NOTE % 12, octave[octaveIndex], MIDI_CH, pitchBendNames[pitchBendIsOn]);

#ifdef DEBUG
              Serial.print("Root: ");
              Serial.println(NOTE);
#endif

              break;
            case 1:
              NOTE++;
              if (NOTE > 60) NOTE = 60;
              printDisplay(scaleNames[scaleIndex], NOTE % 12, octave[octaveIndex], MIDI_CH, pitchBendNames[pitchBendIsOn]);

#ifdef DEBUG
              Serial.print("Root: ");
              Serial.println(NOTE);
#endif

              break;
          }

        }
        buttonPState[index[i]] = buttonCState[index[i]];
      }
    }
  }
}
