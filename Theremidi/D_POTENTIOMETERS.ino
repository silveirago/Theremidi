
/////////////////////////////////////////////
void readPotentiometers(int i) {


  // one pole filter
  // y[n] = A0 * x[n] + B1 * y[n-1];
  // A = 0.15 and B = 0.85

  reading[i] = analogRead(POT_ARDUINO_PIN[i]); // raw reading
  ave[i].push(reading[i]); // adds value to average pool

  //float filteredVal = filterA * reading + filterB * potPState[i]; // filtered value
  filteredVal[i] = ave[i].mean();
  potCState[i] = filteredVal[i];

  //fscale( float originalMin, float originalMax, float newBegin, float newEnd, float inputValue, float curve) - linearize curve
  scaledVal[i] = fscale(IR_range[0], IR_range[1], IR_min_val[i], IR_max_val[i], filteredVal[i], IR_curve);

  pitchBendVal = fscale(IR_range[0], IR_range[1], 0, 16383, filteredVal[i], IR_curve);

  // sets the value to the desired range
  //rangedVal[i] = map(scaledVal[i], IR_range[0], IR_range[1], IR_min_val[i], IR_max_val[i]);

  // clips IR value to min-max
  IR_val[i] = clipValue(scaledVal[i], IR_min_val[i], IR_max_val[i]);

#ifdef DEBUG
  //Debug only
  //for (int i = 0; i < N_POTS_ARDUINO; i++) {
  //if (i == 1) {
  //
  //    Serial.print(i);
  //    Serial.print(": Reading: ");
  //    Serial.print(reading[i]);
  //    Serial.print(" | ");
  //
  //    Serial.print(": filteredVal: ");
  //    Serial.print(filteredVal[i]);
  //    Serial.print(" | ");
  //
  //    Serial.print(": Scaled val: ");
  //    Serial.print(scaledVal[i]);
  //    Serial.print(" | ");
  //
  //    Serial.print(": IRval: ");
  //    Serial.print(IR_val[i]);
  //    Serial.print(" | ");
  //
  //
  //  }
  //  Serial.println();
#endif


  potVar[i] = abs(potCState[i] - potPState[i]); // Calculates the absolute value between the difference between the current and previous state of the pot

  if (potVar[i] > varThreshold[i]) { // Opens the gate if the potentiometer variation is greater than the threshold
    PTime[i] = millis(); // Stores the previous time
  }

  timer[i] = millis() - PTime[i]; // Resets the timer 11000 - 11000 = 0ms

  if (timer[i] < TIMEOUT[i]) { // If the timer is less than the maximum allowed time it means that the potentiometer is still moving
    potMoving[i] = true;
  }
  else {
    potMoving[i] = false;
  }


}

/////////////////////////////////////////////
// POTENTIOMETERS
void IR0() {

  int i = 0;

  readPotentiometers(i);


  if (potMoving[i] == true) { // If the potentiometer is still moving, send the change control

    if ((millis() - lastDebounceTime[i]) > debounceDelay) {

      if (IR_val[i] != IR_Pval[i]) {
        if (IR_val[i] >= 0) {
          lastDebounceTime[i] = millis();

          if (i == 0) { // CC
#ifdef ATMEGA32U4

            controlChange(MIDI_CH, CC + i, IR_val[i]);//  (channel, CC number,  CC value)
            MidiUSB.flush();
#endif

#ifdef DEBUG
            Serial.print("IR");
            Serial.print(i);
            Serial.print(": ");
            Serial.print(reading[i]);
            Serial.print(" | ");
            Serial.print(IR_val[i]);
            Serial.println("    ");
#endif

          }

          if (i == 1) { // NOTE


#ifdef ATMEGA32U4

            noteOut = NOTE + scaleNotes[scaleIndex][IR_val[i]] + octave[octaveIndex];
            noteOn(MIDI_CH, noteOut, 127);  // channel, note, velocity
            MidiUSB.flush();

            noteOut = NOTE + scaleNotes[scaleIndex][IR_Pval[i]] + octave[octaveIndex];
            noteOn(MIDI_CH, noteOut, 0);  // shuts down previous note
            MidiUSB.flush();
            //Serial.print(scaleNotes[scaleIndex][IR_val[i]]);
            //noteOn(MIDI_CH, NOTE + scaleNotes[scaleIndex][IR_val[i]] + octave[octaveIndex], 127);

#endif

#ifdef DEBUG

            int noteOut = NOTE + scaleNotes[scaleIndex][IR_val[i]] + octave[octaveIndex];

            Serial.print("IR");
            Serial.print(i);
            Serial.print(": ");
            Serial.print(reading[i]);
            Serial.print(" | ");
            Serial.print(noteOut);
            //Serial.print(scaleNotes[0][0]);
            //Serial.print(a);
            Serial.println("    ");
#endif

            note_is_playing = true;
          }


        }
      }
    }

    IR_Pval[i] = IR_val[i];
    potPState[i] = potCState[i]; // Stores the current reading of the potentiometer to compare with the next
  }


}



/////////////////////////////////////////////
// POTENTIOMETERS
void IR1() {

  int i = 1;

  readPotentiometers(i);


  if (potMoving[i] == true) { // If the potentiometer is still moving, send the change control

    if ((millis() - lastDebounceTime[i]) > debounceDelay) {


      if (pitchBendIsOn == false) {

        threadIR1.setInterval(20); // every how many millisiconds

        if (IR_val[i] != IR_Pval[i]) {
          if (IR_val[i] >= 0) {
            lastDebounceTime[i] = millis();


#ifdef ATMEGA32U4

            noteOut = NOTE + scaleNotes[scaleIndex][IR_val[i]] + octave[octaveIndex];
            noteOn(MIDI_CH, noteOut, 127);  // channel, note, velocity
            MidiUSB.flush();

            noteOut = NOTE + scaleNotes[scaleIndex][IR_Pval[i]] + octave[octaveIndex];
            noteOn(MIDI_CH, noteOut, 0);  // shuts down previous note
            MidiUSB.flush();
            //Serial.print(scaleNotes[scaleIndex][IR_val[i]]);
            //noteOn(MIDI_CH, NOTE + scaleNotes[scaleIndex][IR_val[i]] + octave[octaveIndex], 127);

#endif

#ifdef DEBUG

            int noteOut = NOTE + scaleNotes[scaleIndex][IR_val[i]] + octave[octaveIndex];

            Serial.print("IR");
            Serial.print(i);
            Serial.print(": ");
            Serial.print(reading[i]);
            Serial.print(" | ");
            Serial.print(noteOut);
            //Serial.print(scaleNotes[0][0]);
            //Serial.print(a);
            Serial.println("    ");
#endif

            note_is_playing = true;


          }
        }
        // shuts down the last note when there's nothing in the sensor

        if (filteredVal[i] < IR_range[0]) {

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

      else { // if pitch bend is on

        if (pitchBendPVal != pitchBendVal) {

          if (pitchBendIsPlaying == false) {

            noteOut = NOTE + octave[octaveIndex];

#ifdef ATMEGA32U4
            noteOn(MIDI_CH, noteOut, 127);  // channel, note, velocity
            MidiUSB.flush();
#endif

            pitchBendIsPlaying = true; // pitch bend is playing

#ifdef DEBUG
            Serial.print("pitch bend is ");
            Serial.println(pitchBendIsPlaying);
#endif

          }

#ifdef ATMEGA32U4
          pitchBend(MIDI_CH, pitchBendVal);
          MidiUSB.flush();
#endif

        }

        if (pitchBendVal < 1) {

          if (pitchBendIsPlaying == true) {

            noteOut = NOTE + octave[octaveIndex];

            // shuts down all notes (kill)
            for (int i = 0; i < 127; i++) {

#ifdef ATMEGA32U4
              noteOn(MIDI_CH, i, 0);
              MidiUSB.flush();
#endif

            }

            pitchBendIsPlaying = false;

#ifdef DEBUG
            Serial.print("pitch bend is ");
            Serial.println(pitchBendIsPlaying);
#endif

          }
        }
      }
    }

    IR_Pval[i] = IR_val[i];
    potPState[i] = potCState[i]; // Stores the current reading of the potentiometer to compare with the next
    pitchBendPVal = pitchBendVal;
  }
}
