#pragma once

#include <Arduino.h>

#undef min
#undef max

#include <functional>

// Handles sending formatted commands and processing received commands via SerialUSB.
class SerialCommunicator {
public:
    SerialCommunicator();

    // Call this frequently in the main loop to process incoming serial data.
    void processInput();

    // --- Sending Methods ---
    void sendAudioPlay(const char* mode, int index); // mode = "dispatch" or "archive"
    void sendVideoPlay(int index);
    void sendFluxVolume(int fluxIndex, int volume);
    void sendFluxLoop(int fluxIndex);
    void sendStateChange(const char* stateName);
    void sendDebug(const char* message);
    void sendMessage(const String& message);


    // --- Callback Registration (using std::function) ---
    void onModeButtonLedOn(std::function<void()> callback);
    void onModeButtonLedOff(std::function<void()> callback);
    void onAudioStopped(std::function<void()> callback);

private:
    String inputBuffer_; // Buffer for assembling incoming lines

    // Callback storage (using std::function)
    std::function<void()> modeButtonLedOnCallback_;
    std::function<void()> modeButtonLedOffCallback_;
    std::function<void()> audioStoppedCallback_;

    // Internal parsing helper
    void parseCommand(const String& command);

    // Define the specific string expected to signal audio stop
    const String STARTED_PLAYING_AUDIO_MSG = "MODE_BUTTON_ON";
    const String STOPPED_PLAYING_AUDIO_MSG = "MODE_BUTTON_OFF";
};
