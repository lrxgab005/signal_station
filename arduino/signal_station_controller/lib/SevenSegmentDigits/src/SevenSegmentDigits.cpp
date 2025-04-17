#include "SevenSegmentDigits.h"

SevenSegmentDigits::SevenSegmentDigits(int numDigits, const int* digitOnPins, const int segmentPins[7])
  : _numDigits(numDigits), currentValue(0), currentDigit(0) {
  _digitOnPins = new int[_numDigits];
  for (int i = 0; i < _numDigits; i++) {
    _digitOnPins[i] = digitOnPins[i];
  }
  for (int i = 0; i < 7; i++) {
    _segmentPins[i] = segmentPins[i];
  }
  _divisors = new int[_numDigits];
  for (int i = 0; i < _numDigits; i++) {
    int power = 1;
    for (int j = 0; j < (_numDigits - 1 - i); j++) {
      power *= 10;
    }
    _divisors[i] = power;
  }
}

SevenSegmentDigits::~SevenSegmentDigits() {
  delete [] _digitOnPins;
  delete [] _divisors;
}

void SevenSegmentDigits::begin() {
  for (int i = 0; i < _numDigits; i++) {
    pinMode(_digitOnPins[i], OUTPUT);
    digitalWrite(_digitOnPins[i], LOW);
  }
  for (int i = 0; i < 7; i++) {
    pinMode(_segmentPins[i], OUTPUT);
    digitalWrite(_segmentPins[i], HIGH);
  }
}

void SevenSegmentDigits::updateValue(int value) {
  currentValue = (value < 0) ? -value : value;
}

void SevenSegmentDigits::refresh() {
  int divisor = _divisors[currentDigit];
  int digitVal = (currentValue / divisor) % 10;
  // Disable all digits to avoid ghosting
  for (int i = 0; i < _numDigits; i++) {
    digitalWrite(_digitOnPins[i], LOW);
  }
  byte pattern = _digitPatterns[digitVal];
  for (int i = 0; i < 7; i++) {
    bool segOn = pattern & (1 << i);
    digitalWrite(_segmentPins[i], segOn ? LOW : HIGH);
  }
  digitalWrite(_digitOnPins[currentDigit], HIGH);
  currentDigit = (currentDigit + 1) % _numDigits;
}

void SevenSegmentDigits::turnOff() {
  for (int i = 0; i < _numDigits; i++) {
    digitalWrite(_digitOnPins[i], HIGH);
    for (int j = 0; j < 7; j++) {
      digitalWrite(_segmentPins[j], HIGH);
    }
  }
}
