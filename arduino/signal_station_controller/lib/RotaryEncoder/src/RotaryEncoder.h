#pragma once

#include <Arduino.h>

/// Callback type used to signal activity from the encoder.
typedef void (*ActivityCallback)();

class RotaryEncoder {
 public:
  // Constructor
  RotaryEncoder(int clk, int dt, int sw, int maxNum);

  // Initializes pin modes. Call this in setup().
  // Note: attachInterrupt() should be called externally in setup().
  void begin();

  // Polls the button pin for debouncing and triggers activity callback.
  // Call this frequently in the main loop(). Rotation is handled by interrupts.
  void update();

  // Gets the current accumulated encoder value (updated by ISR).
  int getEncoderValue();

  // Checks if the button was pressed since the last call to update().
  bool isButtonPressed();

  // Sets the maximum value for the counter (exclusive).
  void setMaxNumber(int maxNum);

  // Resets the internal counter to zero.
  void resetCounter();

  // Registers a callback function for activity detection.
  void setActivityCallback(ActivityCallback cb);

  // --- Public method to be called from ISR ---
  // This processes the encoder pins. MUST be ISR-safe (no delays, Serial, etc.).
  // Make it IRAM_ATTR on ESP32 if needed. Not standard Arduino, but good practice.
  void processEncoderPins();

 private:
  // Member variables using consistent naming
  const int CLK_; // Note: Made const as pins shouldn't change after construction
  const int DT_;
  const int SW_;
  int maxNumber_;
  volatile int counter_; // Volatile because it's changed by ISR, read by main code
  volatile byte encoderState_; // Volatile: shared between ISR and potentially main thread (though only written in ISR context here)
  volatile bool rotationOccurred_; // Flag set by ISR handler, cleared by update

  // Button state (handled by polling in update())
  bool buttonPressedEvent_;
  byte buttonState_;
  unsigned long lastButtonDebounceTime_;

  // Callback
  ActivityCallback activityCallback_;

  // Constants
  static const unsigned long DEBOUNCE_DELAY_MS = 50;
};
