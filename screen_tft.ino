#include <SPI.h>
#include <TFT_eSPI.h> 

/*
HOW TO PLUG SCREEN
  VCC and LED:  3.3V
  GND:          Connected to the GND of Arduino
  CS:           D15
  RESET:        D4
  DC:           D2
  SOI(MOSI):    D23
  SCK:          D18
  SDO(MISO):    D19

TO START THE SERIAL TO MIDI
  Open the serial midi bridge;
  run: source .venv/bin/activate;
  python3 SerialMIDIBridge.py;
  */

/*
 * ====================================================================
 * DUAL-SERIAL SYNTHESIZER DISPLAY (ESP32 SIDE)
 * ====================================================================
 * How it works:
 * 1. Hardware Serial2 (Pins 16 & 17): Listens at 9600 baud for messages 
 * sent from the Nano's SoftwareSerial pin. 
 * 2. It parses incoming strings (e.g., "R,4" or "S,1"), extracts the integer, 
 * maps it to text, and updates the TFT display immediately.
 * 3. USB Serial (Serial): Running at 115200 baud strictly for your Mac's 
 * Serial Monitor debugging.
 */

// Invoke custom library configuration for your specific TFT display pins
TFT_eSPI tft = TFT_eSPI(); 

// Text lookup arrays to translate raw array indices back into human-readable text
const char* rootNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
const char* scaleTypeNames[] = {"Major", "Minor", "Harmonic", "Melodic", "Dorian", "Mixolydian", "Blues"};

// Local tracking variables holding the current index state of the system
int displayRoot = 0;   // Matches index matching rootNames (0 = C)
int displayScale = 0;  // Matches index matching scaleTypeNames (0 = Major)

// DISPLAY REFRESH ENGINE
// Clears the screen canvas and prints the active music configuration strings
void updateScreenDisplay() {
  // Clear the screen completely to prevent pixel ghosting/overlapping text
  tft.fillScreen(TFT_BLACK);
  
  // Draw static header label text
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(1);
  tft.drawString("CURRENT KEY:", 10, 20);

  // Set style for the dynamic scale text string display
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setTextSize(2);
  
  // Combine the root name and scale type into a single printable string (e.g., "C# Dorian")
  String fullScaleText = String(rootNames[displayRoot]) + " " + String(scaleTypeNames[displayScale]);
  tft.drawString(fullScaleText, 10, 40);
}

void setup() {
  Serial.begin(115200); // Open core USB link to your Mac for debugging logs
  
  // Initialize the screen hardware
  tft.init();
  tft.setRotation(1);   // Set screen orientation to wide landscape mode
  
  // Draw initial "C Major" screen elements on power-up
  updateScreenDisplay();

  /*
   * Initialize Secondary Hardware UART Port (Serial2)
   * Speed: 9600 baud (perfectly matches Nano's SoftwareSerial)
   * Config: SERIAL_8N1 (Standard 8 data bits, no parity, 1 stop bit)
   * Pins: RX2 = GPIO 16 (connected to Nano D2), TX2 = GPIO 17 (unused)
   */
  Serial2.begin(9600, SERIAL_8N1, 16, 17); 
}

void loop() {
  // Check if bytes are sitting in the Serial2 hardware receive buffer queue
  if (Serial2.available() > 0) {
    // Read the stream block until it encounters a newline character (\n)
    String inputData = Serial2.readStringUntil('\n');
    
    // Trim away whitespace, carriage returns (\r), or hidden transmission characters
    inputData.trim();
    
    // Echo the data out to your Mac's Serial Monitor for diagnostic confirmation
    Serial.print("ESP32 Received: ");
    Serial.println(inputData);

    // STRING PARSING ENGINE
    // Case 1: Check if the string packet begins with the Root prefix identifier "R,"
    if (inputData.startsWith("R,")) {
      // Strip away "R," prefix, cast remaining numbers to an integer index, and update display tracking state
      displayRoot = inputData.substring(2).toInt();
      updateScreenDisplay();
    }
    // Case 2: Check if the string packet begins with the Scale prefix identifier "S,"
    else if (inputData.startsWith("S,")) {
      // Strip away "S," prefix, cast remaining numbers to an integer index, and update display tracking state
      displayScale = inputData.substring(2).toInt();
      updateScreenDisplay();
    }
  }
}