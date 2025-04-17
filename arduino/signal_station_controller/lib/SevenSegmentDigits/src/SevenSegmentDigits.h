#pragma once

#include <Arduino.h>

class SevenSegmentDigits {
public:
  SevenSegmentDigits(int numDigits, const int* digitOnPins, const int segmentPins[7]);
  ~SevenSegmentDigits();
  
  void begin();
  void updateValue(int value);
  void refresh();
  void turnOff();
  
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
  int* _digitOnPins;
  int _segmentPins[7];
  int _numDigits;
  int* _divisors;
  int currentValue;
  int currentDigit;
};
