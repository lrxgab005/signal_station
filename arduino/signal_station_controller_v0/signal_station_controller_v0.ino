#include <Arduino.h>

// Button State ON LED
const int buttonOnStateLed = 14;

// Rotary Encoder Inputs
const int CLK = 2;
const int DT = 3;
const int SW = 4;
const int maxNunmber = 100;

// Segment pins for 7 segments: A, B, C, D, E, F, G
const int segmentPins[7] = {7, 8, 9, 10, 11, 12, 13};

// Digit ON pins for 7-segment display
const int digitOnPins[2] = {6, 5};


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
      if (digitalRead(SW) == LOW) {
        if (millis() - lastButtonPress > 50) {
          return true;
        } 
        lastButtonPress = millis();
      }
      return false;
    }

  private:
    int CLK, DT, SW, maxNumber;
    int counter;
    int lastStateCLK;
    unsigned long lastButtonPress;
};


MultiplexedDisplay display(2, digitOnPins, segmentPins);
RotaryEncoder encoder(CLK, DT, SW, maxNunmber);

void setup() {

  pinMode(buttonOnStateLed, OUTPUT);
  digitalWrite(buttonOnStateLed, LOW);
  
  display.begin();
  encoder.begin();
  
  Serial.begin(9600);
}

void loop() { 
  int encoderValue = encoder.getEncoderValue();
  bool buttonPressed = encoder.isButtonPressed();
  String encoderStatus = String(encoderValue) + ", " + String(buttonPressed ? 1 : 0);

  Serial.println(encoderStatus);


  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    
    if (command == "ON")
      digitalWrite(buttonOnStateLed, HIGH);
    else if (command == "OFF")
      digitalWrite(buttonOnStateLed, LOW); 
  }
  
  display.updateValue(encoderValue);
  display.refresh();
  
  delay(1);
}
