#include "SystemContext.h"
#include "SevenSegmentDigits.h"
#include "RotaryEncoder.h"
#include "SelectLEDs.h"
#include "StateManager.h"
#include "Button.h"
#include "SerialCommunicator.h"
#include <Arduino.h>

// --- Pin definitions and constants ---
const unsigned long FLUX_POTI_UPDATE_INTERVAL = 50;
const unsigned long MIN_POTI_CHANGE = 1;
const int NUM_FLUX_POTIS = 4;
const int fluxPotiPins[NUM_FLUX_POTIS] = {A4, A5, A6, A7};
int lastfluxPotiValues[NUM_FLUX_POTIS] = {-1, -1, -1, -1};
unsigned long lastfluxPotiUpdateTime = 0;

const int disp_or_arch_button = 51;
const int disp_or_arch_button_led = A11;

const int CLK1 = 3;
const int DT1 = 17;
const int SW1 = 16;

const int CLK2 = 2;
const int DT2 = 14;
const int SW2 = 4;

const int controlPanelSegmentsPins[7] = {35, 37, 39, 41, 43, 45, 47};
const int monitorPanelSegmentsPins[7] = {22, 24, 26, 28, 30, 32, 34};
const int controlPanelDigitOnPins[2] = {31, 33};
const int monitorPanelDigitOnPins[2] = {36, 38};

const int NR_ARCHIVE = 33;
const int NR_DISPATCHES = 8;
const int NR_VIDEOS = 13;
const int dispatchPins[NR_DISPATCHES] = {12, 11, 10, 9, 8, 7, 6, 5};

const int SLEEP_TIMEOUT_S = 60;

// --- Global objects ---
SevenSegmentDigits controlPanelDisplay(2, controlPanelDigitOnPins, controlPanelSegmentsPins);
SevenSegmentDigits monitorPanelDisplay(2, monitorPanelDigitOnPins, monitorPanelSegmentsPins);
SelectLEDs dispatchSelectLeds(dispatchPins, NR_DISPATCHES);
RotaryEncoder encoderAudioState(CLK1, DT1, SW1, NR_ARCHIVE);
RotaryEncoder encoderVideoState(CLK2, DT2, SW2, NR_VIDEOS);
Button toggleButton(disp_or_arch_button);

// Global System Context - Definition
SystemContext systemContext = {
  0, // lastActivityTime
  0, // audioSelected
  -1, // audioPlaying
  0, // videoSelected
  -1, // videoPlaying
  SLEEP_TIMEOUT_S // sleepTimeoutS
};

// Global State Manager instance
StateManager stateManager(&systemContext);

// Global Serial Communicator instance
SerialCommunicator serialComm;


// --- Interrupt Service Routines (ISRs) ---
void ISR_EncoderAudio() {
  encoderAudioState.processEncoderPins();
}
void ISR_EncoderVideo() {
  encoderVideoState.processEncoderPins();
}


// --- UI Callback Functions (Called on State Entry) ---
void onSleep() {
  serialComm.sendStateChange("Sleep");
  dispatchSelectLeds.turnOff();
  controlPanelDisplay.turnOff();
  monitorPanelDisplay.turnOff();
  encoderAudioState.resetCounter();
  encoderVideoState.resetCounter();
}

void onDispatch() {
  serialComm.sendStateChange("Dispatch");
  encoderAudioState.resetCounter();
  dispatchSelectLeds.turnOff();
  dispatchSelectLeds.setLED(systemContext.audioSelected % NR_DISPATCHES, 255);
  controlPanelDisplay.turnOff();
}

void onArchive() {
  serialComm.sendStateChange("Archive");
  encoderAudioState.resetCounter();
  controlPanelDisplay.updateValue(systemContext.audioSelected);
  controlPanelDisplay.refresh();
  dispatchSelectLeds.turnOff();
}


// --- Arduino Setup ---
void setup() {
  SerialUSB.begin(9600);
  // while (!SerialUSB); // Optional wait

  // Initialize hardware components
  encoderAudioState.begin();
  encoderVideoState.begin();
  controlPanelDisplay.begin();
  monitorPanelDisplay.begin();
  dispatchSelectLeds.begin();

  pinMode(disp_or_arch_button_led, OUTPUT);
  digitalWrite(disp_or_arch_button_led, LOW);

  // --- Configure StateManager ---
  encoderAudioState.setActivityCallback([] { stateManager.handleActivity(); });
  encoderVideoState.setActivityCallback([] { stateManager.handleActivity(); });
  toggleButton.setCallback([] { stateManager.handleToggle(); });

  stateManager.registerDispatchEntryCallback(onDispatch);
  stateManager.registerArchiveEntryCallback(onArchive);
  stateManager.registerSleepEntryCallback(onSleep);

  // --- Configure SerialCommunicator Callbacks ---
  serialComm.onModeButtonLedOn([] {
      digitalWrite(disp_or_arch_button_led, HIGH);
  });
  serialComm.onModeButtonLedOff([] {
      digitalWrite(disp_or_arch_button_led, LOW);
  });
  serialComm.onAudioStopped([] {
    if (systemContext.audioPlaying != -1) {
      String msg ="[Audio] Stop detected for index: " + String(systemContext.audioPlaying);
      serialComm.sendDebug(msg.c_str());
      systemContext.audioPlaying = -1;
      if (stateManager.isInState("dispatch")) {
        dispatchSelectLeds.turnOff();
        dispatchSelectLeds.setLED(systemContext.audioSelected % NR_DISPATCHES, 255);
      }
    }
  });

  // --- Attach Interrupts for Encoders ---
  attachInterrupt(digitalPinToInterrupt(CLK1), ISR_EncoderAudio, CHANGE);
  attachInterrupt(digitalPinToInterrupt(DT1), ISR_EncoderAudio, CHANGE);
  attachInterrupt(digitalPinToInterrupt(CLK2), ISR_EncoderVideo, CHANGE);
  attachInterrupt(digitalPinToInterrupt(DT2), ISR_EncoderVideo, CHANGE);

  // Initialize the state machine AFTER everything else.
  stateManager.begin();

  serialComm.sendDebug("System Initialized with Encoder Interrupts.");
}


// --- Logic Functions ---
void fluxPotentiometerLogic() {
  unsigned long currentTime = millis();
  if (currentTime - lastfluxPotiUpdateTime >= FLUX_POTI_UPDATE_INTERVAL) {
    lastfluxPotiUpdateTime = currentTime;
    for (int i = 0; i < NUM_FLUX_POTIS; i++) {
      int rawValue = analogRead(fluxPotiPins[i]);
      int volume = map(rawValue, 0, 1023, 0, 100);
      if (abs(volume - lastfluxPotiValues[i]) > MIN_POTI_CHANGE) {
        lastfluxPotiValues[i] = volume;
        serialComm.sendFluxLoop(i);
        serialComm.sendFluxVolume(i, volume);
      }
    }
  }
}


// --- Arduino Loop ---
void loop() {
  // 1. Update components that rely on polling
  toggleButton.update();
  encoderAudioState.update();
  encoderVideoState.update();

  // 2. Process Serial Input via the communicator
  serialComm.processInput();

  // 3. Update State Machine
  stateManager.update();

  // 4. Handle Actions Based on Input & Update UI (if not sleeping)
  if (!stateManager.isSleeping()) {

    // Check for encoder button presses
    if (encoderAudioState.isButtonPressed()) {
      serialComm.sendAudioPlay(stateManager.currentStateName(), systemContext.audioSelected);
      systemContext.audioPlaying = systemContext.audioSelected;
    }

    if (encoderVideoState.isButtonPressed()) {
      serialComm.sendVideoPlay(systemContext.videoSelected);
      systemContext.videoPlaying = systemContext.videoSelected;
    }

    // Get current encoder values
    int currentAudioSelection = encoderAudioState.getEncoderValue();
    int currentVideoSelection = encoderVideoState.getEncoderValue();

    bool audioChanged = (currentAudioSelection != systemContext.audioSelected);
    bool videoChanged = (currentVideoSelection != systemContext.videoSelected);

    systemContext.audioSelected = currentAudioSelection;
    systemContext.videoSelected = currentVideoSelection;

    // --- Update UI based on current state and context ---
    if (stateManager.isInState("dispatch")) {
      if (audioChanged) {
        dispatchSelectLeds.turnOff();
        dispatchSelectLeds.setLED(systemContext.audioSelected % NR_DISPATCHES, 255);
      }
    } else if (stateManager.isInState("archive")) {
      if (audioChanged) {
        controlPanelDisplay.updateValue(systemContext.audioSelected);
        controlPanelDisplay.refresh();
      }
    }

    if (videoChanged) {
      monitorPanelDisplay.updateValue(systemContext.videoSelected);
    }
  }

  // 5. Process other logic
  fluxPotentiometerLogic();

  // 6. Refresh Outputs
  if (!stateManager.isSleeping()) {
      controlPanelDisplay.refresh();
      monitorPanelDisplay.refresh();
  }
}
