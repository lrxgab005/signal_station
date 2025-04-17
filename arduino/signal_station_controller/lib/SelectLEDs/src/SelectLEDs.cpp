#include "SelectLEDs.h"

SelectLEDs::SelectLEDs(const int* pins, int numLeds, int brightness)
  : _numLeds((numLeds <= MAX_LEDS) ? numLeds : MAX_LEDS) {
  _brightness = constrain(brightness, 0, 255);
  for (int i = 0; i < _numLeds; i++) {
    _pins[i] = pins[i];
  }
}

void SelectLEDs::begin() {
  for (int i = 0; i < _numLeds; i++) {
    pinMode(_pins[i], OUTPUT);
    analogWrite(_pins[i], 0);
  }
}

void SelectLEDs::setLED(int index, int value) {
  if (index >= 0 && index < _numLeds) {
    int scaledValue = (value * _brightness) / 255;
    analogWrite(_pins[index], scaledValue);
  }
}

void SelectLEDs::setAllLEDs(int value) {
  for (int i = 0; i < _numLeds; i++) {
    setLED(i, value);
  }
}

void SelectLEDs::turnOff() {
  setAllLEDs(0);
}
