#pragma once

#include <Arduino.h>

// Workaround: Undefine  conflicting macros before including STL headers
#undef min
#undef max

#include <functional>

// Button class with debouncing and callback functionality.
class Button {
 public:
  explicit Button(uint8_t pin, unsigned long debounceDelayMs = 50);
  void setCallback(std::function<void()> callback);
  void update();

 private:
  uint8_t pin_;
  bool lastStableState_;
  unsigned long lastDebounceTime_;
  const unsigned long debounceDelay_;
  std::function<void()> callback_;
};
