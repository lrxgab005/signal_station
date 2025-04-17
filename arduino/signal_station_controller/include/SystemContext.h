#pragma once
#include <Arduino.h>

// Holds runtime system data (no UI state)
struct SystemContext {
  unsigned long lastActivityTime;
  int audioSelected;
  int audioPlaying;
  int videoSelected;
  int videoPlaying;
  unsigned int sleepTimeoutS;
};
