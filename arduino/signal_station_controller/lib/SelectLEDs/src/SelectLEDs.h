#pragma once

#include <Arduino.h>

class SelectLEDs {
public:
  static const int MAX_LEDS = 16;
  static const int DEFAULT_BRIGHTNESS = 255;

  SelectLEDs(const int* pins, int numLeds, int brightness = DEFAULT_BRIGHTNESS);
  void begin();
  void setLED(int index, int value);
  void setAllLEDs(int value);
  void turnOff();

private:
  int _pins[MAX_LEDS];
  int _numLeds;
  int _brightness;
};
