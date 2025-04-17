#pragma once

#include "SystemContext.h"
#include <Arduino.h>

#undef min
#undef max

#include <functional>
#include <memory>

// Manages state transitions (Dispatch, Archive, Sleep) and associated callbacks.
class StateManager {
 private:
  class State;
  class DispatchState;
  class ArchiveState;
  class SleepState;

 public:
  explicit StateManager(SystemContext* context);
  ~StateManager();

  // Initializes the state machine to its starting state.
  void begin();

  // Updates the state machine; primarily checks for sleep timeout.
  void update();

  // Handles the toggle button press event.
  void handleToggle();

  // Resets the inactivity timer and wakes the system if sleeping.
  void handleActivity();

  // --- Callback Registration ---
  void registerDispatchEntryCallback(std::function<void()> callback);
  void registerArchiveEntryCallback(std::function<void()> callback);
  void registerSleepEntryCallback(std::function<void()> callback);

  // --- State Information ---
  const char* currentStateName() const;
  bool isSleeping() const;
  bool isInState(String stateName);

 private:
  // Performs the transition to the specified new state.
  void transitionTo(State* newState);

  // --- Member Variables ---
  SystemContext* context_;
  std::function<void()> dispatchEntryCallback_;
  std::function<void()> archiveEntryCallback_;
  std::function<void()> sleepEntryCallback_;

  // Pointer to the *current* active state object. This does not own the state.
  State* currentState_;

  // Smart pointers for owning the state objects. Auto-deletion on destruction.
  std::unique_ptr<DispatchState> dispatchState_;
  std::unique_ptr<ArchiveState> archiveState_;
  std::unique_ptr<SleepState> sleepState_;

  // --- Resource Management ---
  StateManager(const StateManager&) = delete;
  StateManager& operator=(const StateManager&) = delete;
};
