#!/bin/bash
cd "$(dirname "$0")"
CM=${CM:-C-m}
SESSION_NAME=${SESSION_NAME:-signal_station}
VENV_ACTIVATE=". env/bin/activate"

SERIAL_SCRIPT="py/serial_to_udp_bridge.py"
AUDIO_SCRIPT="py/audio_player.py"
KODI_SCRIPT="py/kodi_control.py"

AUDIO_PORT=7070
KODI_PORT=7071
ANDROID_IP="192.168.0.33"

# Kill any existing session
tmux kill-session -t $SESSION_NAME 2>/dev/null || true

# Create new tmux session
tmux new-session -d -s $SESSION_NAME

# # Create 2-column layout (we'll only use the left column)
# tmux split-window -h

# Split LEFT column into 3 panes (vertical stack)
tmux split-window -v
tmux select-pane -U
tmux split-window -v
tmux select-pane -U

# Capture pane IDs
PANES=($(tmux list-panes -F "#{pane_id}"))

# LEFT column commands
tmux send-keys -t ${PANES[0]} "$VENV_ACTIVATE && python3 $SERIAL_SCRIPT --udp_targets 127.0.0.1:$AUDIO_PORT 127.0.0.1:$KODI_PORT" $CM
tmux send-keys -t ${PANES[1]} "$VENV_ACTIVATE && python3 $AUDIO_SCRIPT --udp_bind_port $AUDIO_PORT" $CM
tmux send-keys -t ${PANES[2]} "$VENV_ACTIVATE && python3 $KODI_SCRIPT --udp_bind_port $KODI_PORT --ip $ANDROID_IP" $CM

# Right column left blank (free shell)

# Attach to session
tmux attach-session -t $SESSION_NAME
