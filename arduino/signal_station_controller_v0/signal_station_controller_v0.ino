#include <Arduino.h>

// Dispatch / Archive Button & LED
static bool lastDispButtonState = HIGH;
const int disp_or_arch_button = 53;
const int disp_or_arch_button_led = A14;

// Rotary 1 Encoder Inputs
const int CLK1 = 3;
const int DT1 = 17;
const int SW1 = 16;

// Rotary 2 Encoder Inputs
const int CLK2 = 2;
const int DT2 = 14;
const int SW2 = 4;

// Segment pins for 7 segments: A, B, C, D, E, F, G
const int controlPanelSegmentsPins[7] = {35, 37, 39, 41, 43, 45, 47};
const int monitorPanelSegmentsPins[7] = {22, 24, 26, 28, 30, 32, 34};

// Segment ON pins: Digit 1, Digit 2
const int controlPanelDigitOnPins[2] = {31, 33};
const int monitorPanelDigitOnPins[2] = {36, 38};

// Interface constants
const int NR_ARCHIVE = 81;
const int NR_DISPATCHES = 8;
const int NR_VIDEOS = 3;

// Dispatch LED Array pins 
const int dispatchPins[NR_DISPATCHES] = {12, 11, 10, 9, 8, 7, 6, 5};

// Mode names: dispatch (for 7-seg) and archive (for LED)
enum class DisplayMode {
  DISPATCH,
  ARCHIVE
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

  void setMaxNumber(int maxNum) {
    maxNumber = maxNum;
  }
  
  void resetCounter() {
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
  static const int DEFAULT_BRIGHTNESS = 255;

  LEDArray(const int* pins, int numLeds, int brightness = DEFAULT_BRIGHTNESS) {
    _numLeds = (numLeds <= MAX_LEDS) ? numLeds : MAX_LEDS;
    _brightness = constrain(brightness, 0, 255);
    for (int i = 0; i < _numLeds; i++) {
      _pins[i] = pins[i];
    }
    // Increase PWM frequency for Timer1 (pins 11, 12)
    TCCR1B = TCCR1B & B11111000 | B00000001;
    // Increase PWM frequency for Timer2 (pins 9, 10)
    TCCR2B = TCCR2B & B11111000 | B00000001;
    // Increase PWM frequency for Timer3 (pins 2, 3, 5)
    TCCR3B = TCCR3B & B11111000 | B00000001;
  }

  void begin() {
    for (int i = 0; i < _numLeds; i++) {
      pinMode(_pins[i], OUTPUT);
      analogWrite(_pins[i], 0);
    }
  }

  void setLED(int index, int value) {
    if (index >= 0 && index < _numLeds) {
      int scaledValue = (value * _brightness) / 255;
      analogWrite(_pins[index], scaledValue);
    }
  }

  void setAllLEDs(int value) {
    for (int i = 0; i < _numLeds; i++) {
      setLED(i, value);
    }
  }

  void turnOff() {
    setAllLEDs(0);
  }

private:
  int _pins[MAX_LEDS];
  int _numLeds;
  int _brightness;
};

class StateManager {
public:
  StateManager(RotaryEncoder &encoder, int dispatchMax, int archiveMax)
    : currentMode(DisplayMode::DISPATCH),
      _encoder(encoder),
      _dispatchMax(dispatchMax),
      _archiveMax(archiveMax) {}

  void begin() {
    _encoder.setMaxNumber(_dispatchMax);
  }

  void update(bool toggleButtonPressed) {
    if (toggleButtonPressed) {
      toggleMode();
    }
  }

  void toggleMode() {
    if (currentMode == DisplayMode::DISPATCH) {
      currentMode = DisplayMode::ARCHIVE;
      _encoder.setMaxNumber(_archiveMax);
    } else {
      currentMode = DisplayMode::DISPATCH;
      _encoder.setMaxNumber(_dispatchMax);
    }
    _encoder.resetCounter();
  }

  DisplayMode getMode() const { return currentMode; }
  int getIndex() const { return _encoder.getEncoderValue(); }

private:
    DisplayMode currentMode;
    RotaryEncoder &_encoder;
    const int _dispatchMax;
    const int _archiveMax;
};
  
MultiplexedDisplay controlPanelDisplay(2, controlPanelDigitOnPins, controlPanelSegmentsPins);
MultiplexedDisplay monitorPanelDisplay(2, monitorPanelDigitOnPins, monitorPanelSegmentsPins);

RotaryEncoder encoderAudioState(CLK1, DT1, SW1, NR_ARCHIVE);
RotaryEncoder encoderVideoState(CLK2, DT2, SW2, NR_VIDEOS);

LEDArray dispatchLedArray(dispatchPins, NR_DISPATCHES);

StateManager stateManager(encoderAudioState, NR_DISPATCHES, NR_ARCHIVE); 

void setup() {  
  controlPanelDisplay.begin();
  monitorPanelDisplay.begin();

  encoderAudioState.begin();
  encoderVideoState.begin();

  dispatchLedArray.begin();

  stateManager.begin();

  pinMode(disp_or_arch_button, INPUT_PULLUP);
  pinMode(disp_or_arch_button_led, OUTPUT);

  Serial.begin(9600);
}

void monitorControlLogic() {
  /*   
    Monitor Video Control
  */
  if (encoderVideoState.isButtonPressed()) {
    Serial.println("video, " + String(encoderVideoState.getEncoderValue()));
  }

  monitorPanelDisplay.updateValue(encoderVideoState.getEncoderValue());
  monitorPanelDisplay.refresh();
}

void audioControlLogic() {
  /*   
    Dispatch / Archive Audio Control
  */

  // Read current button state and toggle mode if button was pressed
  bool currentDispButtonState = digitalRead(disp_or_arch_button);

  if (lastDispButtonState == HIGH && currentDispButtonState == LOW) {
    stateManager.toggleMode();
    delay(50);
  }
  lastDispButtonState = currentDispButtonState;

  // Rotary encoderAudioState button sends audio file index serial message
  if (encoderAudioState.isButtonPressed()) {
    String msg = (stateManager.getMode() == DisplayMode::DISPATCH ? "dispatch, " : "archive, ") + String(stateManager.getIndex());
    Serial.println(msg);
  }
  
  // Update dispatch / archive panels based on current mode
  if (stateManager.getMode() == DisplayMode::DISPATCH) {
    dispatchLedArray.turnOff();
    dispatchLedArray.setLED(stateManager.getIndex(), 255);
    controlPanelDisplay.turnOff();
  } else {
    controlPanelDisplay.updateValue(stateManager.getIndex());
    controlPanelDisplay.refresh();
    dispatchLedArray.turnOff();
  }

  // Read serial input to control the button LED
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    
    if (command == "ON")
      digitalWrite(disp_or_arch_button_led, HIGH);
    else if (command == "OFF")
      digitalWrite(disp_or_arch_button_led, LOW); 
  }
}

void loop() {
  monitorControlLogic();
  audioControlLogic();
}
