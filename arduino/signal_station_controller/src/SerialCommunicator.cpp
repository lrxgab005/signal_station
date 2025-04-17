#include "SerialCommunicator.h"
#include <utility> // Required for std::move

SerialCommunicator::SerialCommunicator() :
    inputBuffer_(""),
    modeButtonLedOnCallback_(nullptr),
    modeButtonLedOffCallback_(nullptr),
    audioStoppedCallback_(nullptr)
{
    inputBuffer_.reserve(64); // Pre-allocate buffer capacity
}

// Processes incoming serial data, looking for complete lines.
void SerialCommunicator::processInput() {
    if (!SerialUSB) {
        return; // Native USB not ready
    }
    while (SerialUSB.available() > 0) {
        char receivedChar = SerialUSB.read();
        if (receivedChar == '\n' || receivedChar == '\r') {
            // End of line/command detected
            if (inputBuffer_.length() > 0) {
                inputBuffer_.trim();
                parseCommand(inputBuffer_);
                inputBuffer_ = ""; // Clear buffer for next command
            }
        } else {
            // Append character to buffer. String handles allocation.
            inputBuffer_ += receivedChar;
        }
    }
}

// Parses a complete command string and triggers callbacks.
void SerialCommunicator::parseCommand(const String& command) {
    if (command == STARTED_PLAYING_AUDIO_MSG) {
        if (modeButtonLedOnCallback_) modeButtonLedOnCallback_();
    } else if (command == STOPPED_PLAYING_AUDIO_MSG) {
        if (modeButtonLedOffCallback_) modeButtonLedOffCallback_();
    }
}

void SerialCommunicator::sendAudioPlay(const char* mode, int index) {
    if (SerialUSB) {
        SerialUSB.print(mode);
        SerialUSB.print(", play, ");
        SerialUSB.println(index);
    }
}

void SerialCommunicator::sendVideoPlay(int index) {
     if (SerialUSB) {
        SerialUSB.print("video, ");
        SerialUSB.println(index);
     }
}

void SerialCommunicator::sendFluxVolume(int fluxIndex, int volume) {
     if (SerialUSB) {
        SerialUSB.print("flux_");
        SerialUSB.print(fluxIndex);
        SerialUSB.print(", volume, ");
        SerialUSB.println(volume);
     }
}

void SerialCommunicator::sendFluxLoop(int fluxIndex) {
     if (SerialUSB) {
        SerialUSB.print("flux_");
        SerialUSB.print(fluxIndex);
        SerialUSB.println(", loop, 0");
     }
}

void SerialCommunicator::sendStateChange(const char* stateName) {
     if (SerialUSB) {
        SerialUSB.print("[State] Entering ");
        SerialUSB.println(stateName);
     }
}

void SerialCommunicator::sendDebug(const char* message) {
     if (SerialUSB) {
        SerialUSB.println(message);
     }
}

void SerialCommunicator::sendMessage(const String& message) {
     if (SerialUSB) {
        SerialUSB.println(message);
     }
}


// --- Callback Registration Implementations ---
void SerialCommunicator::onModeButtonLedOn(std::function<void()> callback) {
    modeButtonLedOnCallback_ = std::move(callback);
}

void SerialCommunicator::onModeButtonLedOff(std::function<void()> callback) {
    modeButtonLedOffCallback_ = std::move(callback);
}

void SerialCommunicator::onAudioStopped(std::function<void()> callback) {
    audioStoppedCallback_ = std::move(callback);
}

