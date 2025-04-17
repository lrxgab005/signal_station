#include "Button.h"

Button::Button(uint8_t pin, unsigned long debounceDelayMs)
    : pin_(pin),
      lastStableState_(HIGH), 
      lastDebounceTime_(0),
      debounceDelay_(debounceDelayMs),
      callback_(nullptr) {
  pinMode(pin_, INPUT_PULLUP);
  lastStableState_ = digitalRead(pin_);
}

void Button::setCallback(std::function<void()> callback) {
  callback_ = std::move(callback);
}

void Button::update() {
  bool reading = digitalRead(pin_);

  if (reading != lastStableState_) {
    lastDebounceTime_ = millis();
  }

  if ((millis() - lastDebounceTime_) > debounceDelay_) {
    if (reading != lastStableState_) {
      lastStableState_ = reading;

      if (lastStableState_ == LOW && callback_) {
        callback_();
      }
    }
  }

  bool currentReading = digitalRead(pin_);

  if ((millis() - lastDebounceTime_) > debounceDelay_) {
      if (currentReading != lastStableState_) {
          lastStableState_ = currentReading;

          if (lastStableState_ == LOW && callback_) {
              callback_();
          }
      }
  }
}
