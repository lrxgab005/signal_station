#!/bin/bash
cd "$(dirname "$0")"
CM=${CM:-C-m}
SESSION_NAME=${SESSION_NAME:-signal_station}
VENV_ACTIVATE=". env/bin/activate"

SERIAL_SCRIPT="py/serial_to_udp_bridge.py"
AUDIO_SCRIPT="py/audio_player.py"
KODI_SCRIPT="py/kodi_control.py"

AUDIO_BIND_PORT=7070
KODI_BIND_PORT=7071
AUDIO_SEND_PORT=7072

# Try to get Android IP from config file, fallback to argument
if [ -n "$1" ]; then
    ANDROID_IP=$1
    echo "Using Android IP from argument: $ANDROID_IP"
elif [ -f "config/android_ip.conf" ]; then
    ANDROID_IP=$(cat config/android_ip.conf)
    echo "Using Android IP from config: $ANDROID_IP"
else
    echo "Error: No Android IP provided. Usage: $0 <android_ip>"
    echo "Alternatively, create config/android_ip.conf with the IP"
    exit 1
fi

# Try to get Arduino port from config file
if [ -f "config/arduino_port.conf" ]; then
    ARDUINO_PORT=$(cat config/arduino_port.conf)
    echo "Using Arduino port from config: $ARDUINO_PORT"
else
    echo "Error: No Arduino port config found. Please create config/arduino_port.conf"
    exit 1
fi

# Kill any existing session
tmux kill-session -t $SESSION_NAME 2>/dev/null || true

# Create new tmux session
tmux new-session -d -s $SESSION_NAME

# Split left column into 3 panes (vertical stack)
tmux split-window -v
tmux select-pane -U
tmux split-window -v
tmux select-pane -U

# Capture pane IDs
PANES=($(tmux list-panes -F "#{pane_id}"))

# Auto-restart loops in each pane:
tmux send-keys -t ${PANES[0]} "$VENV_ACTIVATE && while true; do python3 $SERIAL_SCRIPT --udp_send_targets 127.0.0.1:$AUDIO_BIND_PORT 127.0.0.1:$KODI_BIND_PORT --udp_listen_targets 127.0.0.1:$AUDIO_SEND_PORT --serial_port $ARDUINO_PORT; echo \"[$(date)] $SERIAL_SCRIPT crashed. Restarting in 1 second...\"; sleep 1; done" $CM
tmux send-keys -t ${PANES[1]} "$VENV_ACTIVATE && while true; do python3 $AUDIO_SCRIPT --udp_bind_port $AUDIO_BIND_PORT --udp_send_port $AUDIO_SEND_PORT; echo \"[$(date)] $AUDIO_SCRIPT crashed. Restarting in 1 second...\"; sleep 1; done" $CM
tmux send-keys -t ${PANES[2]} "$VENV_ACTIVATE && while true; do python3 $KODI_SCRIPT --udp_bind_port $KODI_BIND_PORT --ip $ANDROID_IP; echo \"[$(date)] $KODI_SCRIPT crashed. Restarting in 1 second...\"; sleep 1; done" $CM

# Attach to session
tmux attach-session -t $SESSION_NAME

