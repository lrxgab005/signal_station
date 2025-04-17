#include "StateManager.h"

#include <Arduino.h>
#include <utility>
#include <memory>


// --- State classes --- 
class StateManager::State {
 public:
  virtual ~State() = default;
  virtual void enter() = 0;
  virtual void exit() = 0;
  virtual void update() = 0;
  virtual const char* name() const = 0;
 protected:
  State() = default;
};

class StateManager::DispatchState : public StateManager::State {
 public:
  explicit DispatchState(StateManager* manager) : manager_(manager) {}
  void enter() override {
    if (manager_->dispatchEntryCallback_) {
      manager_->dispatchEntryCallback_();
    }
  }
  void exit() override {}
  void update() override {}
  const char* name() const override { return "dispatch"; }
 private:
  StateManager* manager_;
};

class StateManager::ArchiveState : public StateManager::State {
 public:
  explicit ArchiveState(StateManager* manager) : manager_(manager) {}
  void enter() override {
    if (manager_->archiveEntryCallback_) {
      manager_->archiveEntryCallback_();
    }
  }
  void exit() override {}
  void update() override {}
  const char* name() const override { return "archive"; }
 private:
  StateManager* manager_;
};

class StateManager::SleepState : public StateManager::State {
 public:
  explicit SleepState(StateManager* manager) : manager_(manager) {}
  void enter() override {
    if (manager_->sleepEntryCallback_) {
      manager_->sleepEntryCallback_();
    }
  }
  void exit() override {}
  void update() override {}
  const char* name() const override { return "Sleep"; }
 private:
  StateManager* manager_;
};


// --- StateManager Method Implementations ---
StateManager::StateManager(SystemContext* context)
    : context_(context),
      dispatchEntryCallback_(nullptr),
      archiveEntryCallback_(nullptr),
      sleepEntryCallback_(nullptr),
      currentState_(nullptr),
      dispatchState_(std::unique_ptr<DispatchState>(new DispatchState(this))),
      archiveState_(std::unique_ptr<ArchiveState>(new ArchiveState(this))),
      sleepState_(std::unique_ptr<SleepState>(new SleepState(this))) {
}

StateManager::~StateManager() {
}

void StateManager::begin() {
  if (!context_) {
      if (Serial) {
          Serial.println("ERROR: StateManager context is null in begin()!");
      }
      return;
  }
  context_->lastActivityTime = millis();
  transitionTo(dispatchState_.get());
}

void StateManager::update() {
  if (!currentState_ || !context_) return;

  if (currentState_ != sleepState_.get()) {
      unsigned long inactiveDuration = millis() - context_->lastActivityTime;
      if (inactiveDuration >= (context_->sleepTimeoutS * 1000UL)) {
          context_->audioSelected = 0;
          transitionTo(sleepState_.get());
          return;
      }
  }

  currentState_->update();
}

void StateManager::handleToggle() {
   if (!context_) return;

   handleActivity();

   if (currentState_ == dispatchState_.get()) {
     context_->audioSelected = 0;
     transitionTo(archiveState_.get());
   } else if (currentState_ == archiveState_.get()) {
     context_->audioSelected = 0;
     transitionTo(dispatchState_.get());
   }
}

void StateManager::handleActivity() {
  if (!context_) return;

  context_->lastActivityTime = millis();

  if (currentState_ == sleepState_.get()) {
    context_->audioSelected = 0;
    transitionTo(dispatchState_.get());
  }
}

// --- Callback Registration Implementation ---
void StateManager::registerDispatchEntryCallback(std::function<void()> callback) {
  dispatchEntryCallback_ = std::move(callback);
}

void StateManager::registerArchiveEntryCallback(std::function<void()> callback) {
  archiveEntryCallback_ = std::move(callback);
}

void StateManager::registerSleepEntryCallback(std::function<void()> callback) {
  sleepEntryCallback_ = std::move(callback);
}

// --- Private Helper Methods ---
void StateManager::transitionTo(StateManager::State* newState) {
  if (!newState || currentState_ == newState) {
      return;
  }

  if (currentState_) {
    currentState_->exit();
  }

  currentState_ = newState;
  currentState_->enter();
}

// --- State Information Implementation ---
const char* StateManager::currentStateName() const {
  return currentState_ ? currentState_->name() : "None";
}

bool StateManager::isSleeping() const {
  return currentState_ == sleepState_.get();
}

bool StateManager::isInState(String stateName) {
  if (!currentState_) return false;
  return (stateName.equalsIgnoreCase(currentState_->name()));
}