#include <Arduino.h>
#include <SoftwareSerial.h> 

/*
HOW TO PLUG THE BUTTONS
  keys: D3-D10
  Button to chnge key: D11
  Button to chnge scale: D12
*/


/*
 * ====================================================================
 * DUAL-SERIAL SYNTESIZER CONTROLLER (ARDUINO NANO SIDE)
 * ====================================================================
 * How it works:
 * 1. Hardware Serial (Serial): Talks to your Mac at 38400 baud. It handles 
 * MIDI Note On/Off commands natively for FL Studio via your MIDI bridge.
 * 2. Software Serial (espSerial): Emulates a TX serial port on Pin 2 at 9600 baud. 
 * It sends lightweight scale/root change markers exclusively to the ESP32 screen.
 * This dual setup prevents screen updates from freezing FL Studio.
 */

// Define the SoftwareSerial port (Rx is -1/unused since the Nano only sends data to the screen)
SoftwareSerial espSerial(-1, 2); 

/* * MUSIC THEORY LOOKUP TABLES (CHROMATIC LOOKUP GRIDS)
 * Index mapping: 0=C, 1=C#, 2=D, 3=D#, 4=E, 5=F, 6=F#, 7=G, 8=G#, 9=A, 10=A#, 11=B
 * Each row represents a scale root note. The 7 numbers in the row represent the 
 * relative chromatic steps away from C for every note in that specific scale.
 */
const int majorNotes[12][7] = {
  {0, 2, 4, 5, 7, 9, 11}, {1, 3, 5, 6, 8, 10, 0}, {2, 4, 6, 7, 9, 11, 1},
  {3, 5, 7, 8, 10, 0, 2}, {4, 6, 8, 9, 11, 1, 3}, {5, 7, 9, 10, 0, 2, 4},
  {6, 8, 10, 11, 1, 3, 5}, {7, 9, 11, 0, 2, 4, 6}, {8, 10, 0, 1, 3, 5, 7},
  {9, 11, 1, 2, 4, 6, 8}, {10, 0, 2, 3, 5, 7, 9}, {11, 1, 3, 4, 6, 8, 10}
};
const int minorNotes[12][7] = {
  {0, 2, 3, 5, 7, 8, 10}, {1, 3, 4, 6, 8, 9, 11}, {2, 4, 5, 7, 9, 10, 0},
  {3, 5, 6, 8, 10, 11, 1}, {4, 6, 7, 9, 11, 0, 2}, {5, 7, 8, 10, 0, 1, 3},
  {6, 8, 9, 11, 1, 2, 4}, {7, 9, 10, 0, 2, 3, 5}, {8, 10, 11, 1, 3, 4, 6},
  {9, 11, 0, 1, 2, 4, 7}, {10, 0, 1, 3, 5, 6, 8}, {11, 1, 2, 4, 6, 7, 9}
};
const int harmonicMinorNotes[12][7] = {
  {0, 2, 3, 5, 7, 8, 11}, {1, 3, 4, 6, 8, 9, 0}, {2, 4, 5, 7, 9, 10, 1},
  {3, 5, 6, 8, 10, 11, 2}, {4, 6, 7, 9, 11, 0, 3}, {5, 7, 8, 10, 0, 1, 4},
  {6, 8, 9, 11, 1, 2, 5}, {7, 9, 10, 0, 2, 3, 6}, {8, 10, 11, 1, 3, 4, 7},
  {9, 11, 0, 1, 2, 5, 8}, {10, 0, 1, 3, 5, 6, 9}, {11, 1, 2, 4, 6, 7, 10}
};
const int melodicMinorNotes[12][7] = {
  {0, 2, 3, 5, 7, 9, 11}, {1, 3, 4, 6, 8, 10, 0}, {2, 4, 5, 7, 9, 11, 1},
  {3, 5, 6, 8, 10, 0, 2}, {4, 6, 7, 9, 11, 1, 3}, {5, 7, 8, 10, 0, 2, 4},
  {6, 8, 9, 11, 1, 3, 5}, {7, 9, 10, 0, 2, 4, 6}, {8, 10, 11, 1, 3, 5, 7},
  {9, 11, 0, 1, 2, 6, 8}, {10, 0, 1, 3, 5, 7, 9}, {11, 1, 2, 4, 6, 9, 10}
};
const int dorianNotes[12][7] = {
  {0, 2, 3, 5, 7, 9, 10}, {1, 3, 4, 6, 8, 10, 11}, {2, 4, 5, 7, 9, 11, 0},
  {3, 5, 6, 8, 10, 0, 1}, {4, 6, 7, 9, 11, 1, 2}, {5, 7, 8, 10, 0, 2, 3},
  {6, 8, 9, 11, 1, 3, 4}, {7, 9, 10, 0, 2, 4, 5}, {8, 10, 11, 1, 3, 5, 6},
  {9, 11, 0, 1, 2, 6, 7}, {10, 0, 1, 3, 5, 7, 8}, {11, 1, 2, 4, 6, 9, 0}
};
const int mixolydianNotes[12][7] = {
  {0, 2, 4, 5, 7, 9, 10}, {1, 3, 5, 6, 8, 10, 11}, {2, 4, 6, 7, 9, 11, 0},
  {3, 5, 7, 8, 10, 0, 1}, {4, 6, 8, 9, 11, 1, 2}, {5, 7, 9, 10, 0, 2, 3},
  {6, 8, 10, 11, 1, 3, 4}, {7, 9, 11, 0, 2, 4, 5}, {8, 10, 0, 1, 3, 5, 6},
  {9, 11, 1, 2, 4, 6, 7}, {10, 0, 2, 3, 5, 7, 8}, {11, 1, 3, 4, 6, 9, 10}
};
const int bluesNotes[12][7] = {
  {0, 3, 5, 6, 7, 10, 0}, {1, 4, 6, 7, 8, 11, 1}, {2, 5, 7, 8, 9, 0, 2},
  {3, 6, 8, 9, 10, 1, 3}, {4, 7, 9, 10, 11, 2, 4}, {5, 8, 10, 11, 0, 3, 5},
  {6, 9, 11, 0, 1, 4, 6}, {7, 10, 0, 1, 2, 5, 7}, {8, 11, 1, 2, 3, 6, 8},
  {9, 0, 2, 3, 4, 7, 9}, {10, 1, 3, 4, 5, 8, 10}, {11, 2, 4, 5, 6, 9, 11}
};

// Master collection of array pointers to cycle through the 7 scale types easily
const int* scales[7] = {
  (const int*)majorNotes, (const int*)minorNotes, (const int*)harmonicMinorNotes,
  (const int*)melodicMinorNotes, (const int*)dorianNotes, (const int*)mixolydianNotes, (const int*)bluesNotes
};

// HARDWARE PIN INITIALIZATION
const int keys[] = {3, 4, 5, 6, 7, 8, 9, 10}; // 8 musical instrument keys
const int buttonChangeRootNote = 11;          // Cycle root keys (C, C#, D...)
const int buttonChangeScaleType = 12;         // Cycle types (Major, Minor...)

// STATE TRACKING INDICES
int currentScaleNotes[7];                     // Holds the 7 notes of the currently active scale
int currentRootNote = 0;                       // Starts at Index 0 ("C")
int currentScaleType = 0;                      // Starts at Index 0 ("Major")  
int activeMidiNotes[8] = {-1, -1, -1, -1, -1, -1, -1, -1}; // Tracks active notes on keys to prevent loop repetition

// MIDI RAW BYTE WRITER
void sendMIDI(byte command, byte note, byte velocity) {
  Serial.write(command);   // e.g., 0x90 (Note On channel 1) or 0x80 (Note Off)
  Serial.write(note);      // MIDI Note raw conversion values (0-127)
  Serial.write(velocity);  // Auditory amplitude / velocity weight (0-127)
}

// SMART SCALE WRAPAROUND HANDLER
// Translates musical notation indices safely into raw MIDI notes centered around Middle C (60)
int getPlayableMidiNote(int noteIndex, bool forceOctaveUp = false) {
  int midiNote = 60 + noteIndex;
  
  // Wrap handling: If a scale note loops below the selected root pitch, 
  // push it up a full octave to maintain upward structural harmony.
  if (noteIndex < currentRootNote) midiNote += 12;
  
  // Used to force the 8th key to complete the octave wrap seamlessly
  if (forceOctaveUp) midiNote += 12;
  return midiNote;
}

// PULL ACTIVE RAW SCALE PROFILE DATA INTO WORKING BUFFER
void updateActiveScale() {
  // Select the specific sub-row matching the active root within the chosen scale matrix
  const int* selectedRow = scales[currentScaleType] + (currentRootNote * 7);
  for (int i = 0; i < 7; i++) {
    currentScaleNotes[i] = selectedRow[i];
  }
}

void setup() {
  Serial.begin(38400);       // Establish hardware streaming lane to Mac Bridge
  espSerial.begin(9600);     // Establish Software Serial stream to ESP32 screen
  
  // Set up utility buttons and piano keys using internal pullup configurations
  pinMode(buttonChangeRootNote, INPUT_PULLUP);
  pinMode(buttonChangeScaleType, INPUT_PULLUP);
  for (int i = 0; i < 8; i++) {
    pinMode(keys[i], INPUT_PULLUP);
  }
  
  // Initialize the device with default C Major configuration arrays
  updateActiveScale();
}

// KEY SCAN AND ENGINE TRIGGER
void playNote() {
  for (int i = 0; i < 8; i++) {
    // Read inverted input values natively generated by INPUT_PULLUP architecture
    bool isPressed = (digitalRead(keys[i]) == LOW);
    
    if (isPressed) {
      int targetMidiNote = -1;
      
      // Calculate dynamic MIDI mappings
      if (i == 7) {
        // The 8th key repeats the root key pitch raised one full octave up
        targetMidiNote = getPlayableMidiNote(currentScaleNotes[0], true);
      } else {
        targetMidiNote = getPlayableMidiNote(currentScaleNotes[i], false);
      }

      // Check if state profile has transitioned or changed on this specific button 
      if (activeMidiNotes[i] != targetMidiNote) {
        // Cut off any note that was previously mapped to this key
        if (activeMidiNotes[i] != -1) sendMIDI(0x80, activeMidiNotes[i], 0);
        
        // Trigger Note On at maximum velocity (127)
        sendMIDI(0x90, targetMidiNote, 127);
        activeMidiNotes[i] = targetMidiNote; // Cache state signature
      }
    } else {
      // Key released state action loop
      if (activeMidiNotes[i] != -1) {
        sendMIDI(0x80, activeMidiNotes[i], 0); // Send cleaner clean-up Note Off instruction
        activeMidiNotes[i] = -1;               // Clear tracking register data
      }
    }
  }
}

void loop() {
  // BUTTON POLLED MONITORING: SCALE SYSTEM MODULATION
  if (digitalRead(buttonChangeScaleType) == LOW) { 
    currentScaleType++;
    if (currentScaleType >= 7) currentScaleType = 0; // Wrap back to index 0 (Major)
    updateActiveScale();
    
    // Pipe a clean update signature to ESP32 via SoftwareSerial out of Pin 2
    espSerial.print("S,"); 
    espSerial.println(currentScaleType); 
    
    delay(500); // Mechanical hardware debounce filter protection window
  }

  // BUTTON POLLED MONITORING: ROOT KEY VARIATION MODULATION
  if (digitalRead(buttonChangeRootNote) == LOW) {
    currentRootNote++;
    if (currentRootNote >= 12) currentRootNote = 0; // Wrap back to index 0 (C)
    updateActiveScale();
    
    // Pipe a clean update signature to ESP32 via SoftwareSerial out of Pin 2
    espSerial.print("R,"); 
    espSerial.println(currentRootNote); 
    
    delay(500); // Mechanical hardware debounce filter protection window
  }

  // Continuously scan piano keys for note plays
  playNote();
  delay(5); // Core runtime processing delay cushion
}