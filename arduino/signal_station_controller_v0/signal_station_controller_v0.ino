#include <Arduino.h>

// Dispatch / Archive LED Button
const int disp_or_arch_button = 53;
const int disp_or_arch_button_led = A14;

// Rotary 1 Encoder Inputs
const int CLK1 = 18;
const int DT1 = 17;
const int SW1 = 16;
const int ROT_NR_1 = 8;

// Rotary 2 Encoder Inputs
const int CLK2 = 2;
const int DT2 = 3;
const int SW2 = 4;
const int ROT_NR_2 = 3;

// Segment pins for 7 segments: A, B, C, D, E, F, G
const int segmentPins[7] = {35, 37, 39, 41, 43, 45, 47};
const int digitOnPins[2] = {31, 33};

// Digit ON pins for 7-segment display
const int NUM_LEDS = 8;
const int dispatchPins[NUM_LEDS] = {12, 11, 10, 9, 8, 7, 6, 5};

// Display modes
enum class DisplayMode {
  SEGMENT,
  LED
};

class MultiplexedDisplay {
  public:
    MultiplexedDisplay(int numDigits, const int* digitOnPins, const int segmentPins[7]) {
      _numDigits = numDigits;
      _digitOnPins = new int[_numDigits];
      for (int i = 0; i < _numDigits; i++) {
        _digitOnPins[i] = digitOnPins[i];
      }
      for (int i = 0; i < 7; i++) {
        _segmentPins[i] = segmentPins[i];
      }
      currentValue = 0;
      currentDigit = 0;
      // Precompute divisors for digit extraction: divisor[i] = 10^(numDigits-1-i)
      _divisors = new int[_numDigits];
      for (int i = 0; i < _numDigits; i++) {
        int power = 1;
        for (int j = 0; j < (_numDigits - 1 - i); j++) {
          power *= 10;
        }
        _divisors[i] = power;
      }
    }

    ~MultiplexedDisplay() {
      delete [] _digitOnPins;
      delete [] _divisors;
    }

    void begin() {
      for (int i = 0; i < _numDigits; i++) {
        pinMode(_digitOnPins[i], OUTPUT);
        digitalWrite(_digitOnPins[i], LOW);
      }
      for (int i = 0; i < 7; i++) {
        pinMode(_segmentPins[i], OUTPUT);
        digitalWrite(_segmentPins[i], HIGH);
      }
    }

    void updateValue(int value) {
      currentValue = (value < 0) ? -value : value;
    }

    void refresh() {
      // Extract digit value for the current digit position
      int divisor = _divisors[currentDigit];
      int digitVal = (currentValue / divisor) % 10;

      // Disable all digits to prevent ghosting
      for (int i = 0; i < _numDigits; i++) {
        digitalWrite(_digitOnPins[i], LOW);
      }

      // Set segment outputs based on digit pattern (common anode: LOW turns segment on)
      byte pattern = _digitPatterns[digitVal];
      for (int i = 0; i < 7; i++) {
        bool segOn = pattern & (1 << i);
        digitalWrite(_segmentPins[i], segOn ? LOW : HIGH);
      }

      // Enable current digit
      digitalWrite(_digitOnPins[currentDigit], HIGH);
      currentDigit = (currentDigit + 1) % _numDigits;
    }

    void turnOff() {
      for (int i = 0; i < _numDigits; i++) {
        digitalWrite(_digitOnPins[i], HIGH);
        for (int j = 0; j < 7; j++) {
          digitalWrite(_segmentPins[j], HIGH);
        }
        digitalWrite(_digitOnPins[i], LOW);
      }
    }

  private:
    /*
        7-Segment Display Configuration:
            A
           ---
        F |   | B
          | G |
           ---
        E |   | C
          |   |
           ---
            D
    */
    const byte _digitPatterns[10] = {
      0b00111111,  // 0: A B C D E F on
      0b00000110,  // 1: B C on
      0b01011011,  // 2: A B D E G on
      0b01001111,  // 3: A B C D G on
      0b01100110,  // 4: B C F G on
      0b01101101,  // 5: A C D F G on
      0b01111101,  // 6: A C D E F G on
      0b00000111,  // 7: A B C on
      0b01111111,  // 8: all segments on
      0b01101111   // 9: A B C D F G on
    };
    int* _digitOnPins;      // Dynamic array for digit control pins
    int _segmentPins[7];     // Fixed array for segment control pins
    int _numDigits;          // Number of digits in the display
    int* _divisors;          // Precomputed divisors for digit extraction
    int currentValue;        // Absolute value to be displayed
    int currentDigit;        // Current digit being refreshed (0-indexed)
};


class RotaryEncoder {
  public:
    RotaryEncoder(int clk, int dt, int sw, int maxNum)
      : CLK(clk), DT(dt), SW(sw), maxNumber(maxNum), counter(0),
        lastStateCLK(HIGH), lastButtonPress(0) {}

    void begin() {
      pinMode(CLK, INPUT);
      pinMode(DT, INPUT);
      pinMode(SW, INPUT_PULLUP);
      lastStateCLK = digitalRead(CLK);
    }

    int getEncoderValue() {
      int currentStateCLK = digitalRead(CLK);
      if (currentStateCLK != lastStateCLK && currentStateCLK == HIGH) {
        counter += (digitalRead(DT) != currentStateCLK) ? 1 : -1;
        counter = (counter < 0) ? (counter + maxNumber) : (counter % maxNumber);
      }
      lastStateCLK = currentStateCLK;
      return counter;
    }

    bool isButtonPressed() {
      static bool lastButtonState = HIGH;
      bool currentState = digitalRead(SW);
    
      if (lastButtonState == HIGH && currentState == LOW) {
        if (millis() - lastButtonPress > 200) {
          lastButtonPress = millis();
          lastButtonState = currentState;
          return true;
        }
      }
    
      lastButtonState = currentState;
      return false;
    }
    
    void reset() {
      counter = 0;
    }

  private:
    int CLK, DT, SW, maxNumber;
    int counter;
    int lastStateCLK;
    unsigned long lastButtonPress;
};

class LEDArray {
  public:
    static const int MAX_LEDS = 16;
  
    LEDArray(const int* pins, int numLeds) {
      _numLeds = (numLeds <= MAX_LEDS) ? numLeds : MAX_LEDS;
      for (int i = 0; i < _numLeds; i++) {
        _pins[i] = pins[i];
      }
    }
  
    void begin() {
      for (int i = 0; i < _numLeds; i++) {
        pinMode(_pins[i], OUTPUT);
        analogWrite(_pins[i], 0);
      }
    }
  
    void setLED(int index, int value) {
      if (index >= 0 && index < _numLeds) {
        analogWrite(_pins[index], value);
      }
    }

    void setAllLEDs(int value) {
      for (int i = 0; i < _numLeds; i++) {
        analogWrite(_pins[i], value);
      }
    }

    void turnOff() {
      setAllLEDs(0);
    }
  
  private:
    int _pins[MAX_LEDS];
    int _numLeds;
};

class StateManager {
  public:
    StateManager() : 
      currentMode(DisplayMode::SEGMENT),
      currentValue(0),
      currentLedIndex(0) {}

    void begin() {
      pinMode(disp_or_arch_button_led, OUTPUT);
      digitalWrite(disp_or_arch_button_led, HIGH);
    }

    // Minimal change: update currentLedIndex based on encoder value.
    void updateValue(int value) {
      currentValue = value;
      if (currentMode == DisplayMode::LED) {
        currentLedIndex = value % NUM_LEDS;
      }
    }

    void toggleMode() {
      if (currentMode == DisplayMode::SEGMENT) {
        currentMode = DisplayMode::LED;
        digitalWrite(disp_or_arch_button_led, HIGH);
      } else {
        currentMode = DisplayMode::SEGMENT;
        digitalWrite(disp_or_arch_button_led, LOW);
      }
    }

    DisplayMode getMode() const {
      return currentMode;
    }

    int getValue() const {
      return currentValue;
    }

    int getLedIndex() const {
      return currentLedIndex;
    }

  private:
    DisplayMode currentMode;
    int currentValue;
    int currentLedIndex;
};

MultiplexedDisplay display(2, digitOnPins, segmentPins);
RotaryEncoder encoder(CLK1, DT1, SW1, ROT_NR_1);
LEDArray ledArray(dispatchPins, NUM_LEDS);
StateManager stateManager;

void setup() {
  stateManager.begin();
  display.begin();
  encoder.begin();
  ledArray.begin();
  
  Serial.begin(2000000);
}

void loop() {
  int encoderValue = encoder.getEncoderValue();
  bool buttonPressed = encoder.isButtonPressed();
  
  // Update state with encoder value
  stateManager.updateValue(encoderValue);
  
  // Check for button press to toggle mode
  if (buttonPressed) {
    stateManager.toggleMode();
    encoder.reset();
    Serial.println("Mode toggled: " + String(stateManager.getMode() == DisplayMode::SEGMENT ? "SEGMENT" : "LED"));
  }

  // Update display based on current mode and value
  if (stateManager.getMode() == DisplayMode::SEGMENT) {
    display.updateValue(stateManager.getValue());
    display.refresh();
    ledArray.turnOff();
  } else {
    ledArray.turnOff();
    ledArray.setLED(stateManager.getLedIndex(), 255);
    display.turnOff();
  }

  // Check for serial commands
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    
    if (command == "ON")
      digitalWrite(disp_or_arch_button_led, HIGH);
    else if (command == "OFF")
      digitalWrite(disp_or_arch_button_led, LOW); 
  }
  
  // Send current state over serial
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 500) {
    lastPrint = millis();
    String stateInfo = "Mode: " + String(stateManager.getMode() == DisplayMode::SEGMENT ? "SEGMENT" : "LED") +
                       ", Value: " + String(stateManager.getValue());
    Serial.println(stateInfo);
  }
}
