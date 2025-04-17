#include "RotaryEncoder.h"

// Button debounce states
#define BTN_STATE_IDLE 0
#define BTN_STATE_WAIT_LOW 1
#define BTN_STATE_PRESSED 2
#define BTN_STATE_WAIT_HIGH 3


RotaryEncoder::RotaryEncoder(int clk, int dt, int sw, int maxNum)
  : CLK_(clk),
    DT_(dt),
    SW_(sw),
    maxNumber_(maxNum),
    counter_(0),
    // No need to read lastStateCLK here, state machine handles it
    encoderState_(0),
    rotationOccurred_(false),
    buttonPressedEvent_(false),
    buttonState_(BTN_STATE_IDLE),
    lastButtonDebounceTime_(0),
    activityCallback_(nullptr) {}

void RotaryEncoder::begin() {
  pinMode(CLK_, INPUT);
  pinMode(DT_, INPUT);
  pinMode(SW_, INPUT_PULLUP);

  // Read initial state for encoder state machine
  encoderState_ = (digitalRead(CLK_) << 1) | digitalRead(DT_);

  // Initialize button state
  if (digitalRead(SW_) == LOW) {
      buttonState_ = BTN_STATE_PRESSED; // Assume pressed if low at start
  } else {
      buttonState_ = BTN_STATE_IDLE;
  }

  // IMPORTANT: Call attachInterrupt() externally in your main setup() function!
  // Example (in main setup):
  // attachInterrupt(digitalPinToInterrupt(encoderPinCLK), ISR_Wrapper_Func_CLK, CHANGE);
  // attachInterrupt(digitalPinToInterrupt(encoderPinDT), ISR_Wrapper_Func_DT, CHANGE);
}

// This method is designed to be called from an ISR
void RotaryEncoder::processEncoderPins() {
  // Read pins inside ISR for minimal latency
  int sig1 = digitalRead(CLK_);
  int sig2 = digitalRead(DT_);
  int encoded = (sig1 << 1) | sig2;
  int sum = (encoderState_ << 2) | encoded; // Combine prev & current state

  bool rotated = false;
  // Use state transition table to update counter
  if (sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011) {
      counter_++;
      rotated = true;
  } else if (sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000) {
      counter_--;
      rotated = true;
  }

  encoderState_ = encoded; // Save current state for next interrupt

  if (rotated) {
      // Handle wrapping carefully within ISR if needed, or defer complex logic
      // Simple wrap around:
      counter_ = (counter_ % maxNumber_ + maxNumber_) % maxNumber_;
      rotationOccurred_ = true; // Set flag for main loop/update()
  }
}


// Update handles button polling and triggering the activity callback
void RotaryEncoder::update() {
  unsigned long now = millis();
  bool buttonActivity = false;

  // --- Button Debouncing (State Machine) ---
  // This remains polling-based for simplicity and ISR safety
  bool currentButtonReading = (digitalRead(SW_) == LOW);

  buttonPressedEvent_ = false; // Reset event flag each update cycle

  switch (buttonState_) {
    case BTN_STATE_IDLE:
      if (currentButtonReading) {
        buttonState_ = BTN_STATE_WAIT_LOW;
        lastButtonDebounceTime_ = now;
      }
      break;

    case BTN_STATE_WAIT_LOW:
      if (!currentButtonReading) {
        buttonState_ = BTN_STATE_IDLE;
      } else if (now - lastButtonDebounceTime_ >= DEBOUNCE_DELAY_MS) {
        buttonState_ = BTN_STATE_PRESSED;
        buttonPressedEvent_ = true; // Signal the press event ONCE
        buttonActivity = true;    // Button press is activity
      }
      break;

    case BTN_STATE_PRESSED:
      if (!currentButtonReading) {
        buttonState_ = BTN_STATE_WAIT_HIGH;
        lastButtonDebounceTime_ = now;
      }
      break;

    case BTN_STATE_WAIT_HIGH:
      if (currentButtonReading) {
        buttonState_ = BTN_STATE_PRESSED;
      } else if (now - lastButtonDebounceTime_ >= DEBOUNCE_DELAY_MS) {
        buttonState_ = BTN_STATE_IDLE;
      }
      break;
  }

  // --- Trigger Activity Callback ---
  // Check flags set by button logic OR the ISR handler
  bool rotationFlag = rotationOccurred_; // Read volatile flag
  if (rotationFlag) {
      rotationOccurred_ = false; // Clear flag after reading
  }

  if ((buttonActivity || rotationFlag) && activityCallback_) {
    activityCallback_();
  }
}

int RotaryEncoder::getEncoderValue() {
  // Reading volatile int on 32-bit MCU is usually atomic enough
  // For 8-bit MCUs, disable interrupts temporarily if needed:
  // int val;
  // noInterrupts();
  // val = counter_;
  // interrupts();
  // return val;
  return counter_;
}

bool RotaryEncoder::isButtonPressed() {
  // Return the event flag captured during update()
  if (buttonPressedEvent_) {
    buttonPressedEvent_ = false; // Clear the flag after checking
    return true;
  }
  return false;
}

void RotaryEncoder::setMaxNumber(int maxNum) {
  if (maxNum > 0) {
      // Might need interrupt disabling if changing maxNumber while ISR runs
      // noInterrupts();
      maxNumber_ = maxNum;
      counter_ = (counter_ % maxNumber_ + maxNumber_) % maxNumber_;
      // interrupts();
  }
}

void RotaryEncoder::resetCounter() {
  // Might need interrupt disabling
  // noInterrupts();
  counter_ = 0;
  // interrupts();
  // Optionally trigger activity?
  // rotationOccurred_ = true; // Signal change?
}

void RotaryEncoder::setActivityCallback(ActivityCallback cb) {
  activityCallback_ = cb;
}
