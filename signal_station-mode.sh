#!/bin/bash

start_signal_station() {
  # Check if signal-station directory exists
  if [ ! -d ~/signal_station ]; then
    echo "Error: signal_station directory not found in home directory"
    echo "Please clone signal_station repository in your home directory"
    exit 1
  fi

  # Start the signal station
  echo "Starting signal station..."
  cd ~/signal_station || exit 1
  ./run_signal_station.sh
}

stop_signal_station() {
  # Stop the signal station
  echo "Stopping signal station..."
  tmux kill-server
}


case "$1" in
    start)
        start_signal_station
        ;;
    stop)
        stop_signal_station
        ;;
    *)
        echo "Usage: $0 {start|stop}"
        exit 1
esac