import argparse
import socket
import threading
import time
import os
import logging
import pygame

logging.basicConfig(level=logging.INFO,
                    format="%(asctime)s %(levelname)s: %(message)s")

SUPPORTED_FORMATS = ('.wav', '.mp3', '.ogg')


class UDPModeSender:

  def __init__(self, host="127.0.0.1", port=7072):
    self.target = (host, port)
    self.socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    logging.info(f"UDPModeSender: Sending to {self.target}")

  def send(self, message: str):
    try:
      self.socket.sendto(message.encode(), self.target)
      logging.info(f"Sent UDP message to {self.target}: {message}")
    except Exception as e:
      logging.error(f"Error sending UDP message: {e}")


class AudioModule:

  def __init__(self,
               names,
               track_dirs,
               channel_id,
               cooldown=0.5,
               udp_sender=None):
    self.names = names
    self.track_dirs = track_dirs
    self.channel_id = channel_id
    self.cooldown = cooldown
    self.last_command_time = 0
    self.udp_sender = udp_sender
    self.playing = False
    self.sounds = self.load_sounds()
    self.channel = pygame.mixer.Channel(channel_id)
    logging.info(f"Initialized modules {self.names} on channel {channel_id}")

  def load_sounds(self):
    sounds_dict = {}
    for name, track_dir in zip(self.names, self.track_dirs):
      sounds_in_dir = []
      if not os.path.isdir(track_dir):
        logging.warning(f"Directory not found for {name}: {track_dir}")
        sounds_dict[name] = sounds_in_dir
        continue

      for fname in sorted(os.listdir(track_dir)):
        if not fname.lower().endswith(SUPPORTED_FORMATS):
          logging.warning(f"{name}: Skipping unsupported file {fname}")
          continue

        path = os.path.join(track_dir, fname)
        try:
          sound = pygame.mixer.Sound(path)
          sounds_in_dir.append(sound)
          logging.info(f"{name}: Loaded sound {path}")
        except Exception as e:
          logging.error(f"{name}: Error loading {path}: {e}")
      sounds_dict[name] = sounds_in_dir
    return sounds_dict

  def set_volume(self, volume_percent):
    vol = max(0.0, min(1.0, volume_percent / 100.0))
    self.channel.set_volume(vol)
    log_name = self.names[0] if self.names else "UnnamedGroup"
    logging.info(
        f"Group {log_name} (Ch {self.channel_id}): Volume set to {vol}")

  def play_track(self, name, track_index, loop=False):
    now = time.time()
    if now - self.last_command_time < self.cooldown:
      logging.info(
          f"{name} (Group {self.channel_id}): Command ignored due to cooldown")
      return
    self.last_command_time = now

    if (name not in self.sounds or track_index < 0
        or track_index >= len(self.sounds[name])):
      logging.error(
          f"{name} (Group {self.channel_id}): Invalid track index {track_index}"
      )
      return

    if self.channel.get_busy():
      self.channel.stop()
      if self.udp_sender:
        self.udp_sender("MODE_BUTTON_OFF")

    if self.udp_sender:
      self.udp_sender("MODE_BUTTON_ON")

    self.playing = True
    sound = self.sounds[name][track_index]
    self.channel.play(sound, loops=-1 if loop else 0)
    logging.info(
        f"{name} (Group {self.channel_id}): Playing track {track_index}")

    # Pass name arg, though not used in simple monitor version
    threading.Thread(target=self._monitor_playback, args=(name, ),
                     daemon=True).start()

  def _monitor_playback(self, name):
    while self.channel.get_busy():
      time.sleep(0.1)
    if self.udp_sender:
      self.udp_sender("MODE_BUTTON_OFF")
    self.playing = False

  def process_command(self, module_name, command, value):
    if command == "volume":
      self.set_volume(int(value))
    elif command == "play":
      self.play_track(module_name, int(value))
    elif command == "loop":
      self.play_track(module_name, int(value), loop=True)
    elif command == "stop":
      self.channel.stop()
      logging.info(f"{module_name}:{self.channel_id}): Stopped playback")
    else:
      logging.warning(
          f"{module_name}:{self.channel_id}): Unknown command '{command}'")


class MultiChannelController:

  def __init__(self, udp_bind_host, udp_bind_port, modules):
    self.udp_bind_host = udp_bind_host
    self.udp_bind_port = udp_bind_port
    self.modules = modules
    self.udp_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    self.udp_socket.bind((udp_bind_host, udp_bind_port))
    logging.info(f"UDP server listening on {(udp_bind_host, udp_bind_port)}")

  def run(self):
    try:
      while True:
        data, addr = self.udp_socket.recvfrom(1024)
        self.process_message(data)
    except KeyboardInterrupt:
      logging.info("Shutting down UDP server...")
    finally:
      self.udp_socket.close()

  def process_message(self, data):
    try:
      message = data.decode().strip()
      parts = [p.strip().lower() for p in message.split(",")]
      if len(parts) != 3:
        logging.debug(f"Invalid message format: {message}")
        return
      module_name, command, value = parts
      if module_name in self.modules:
        # Pass module_name to the handler
        self.modules[module_name].process_command(module_name, command, value)
      else:
        logging.warning(f"Unknown module: {module_name}")
    except Exception as e:
      logging.error(f"Error processing message: {e}")


def main():
  parser = argparse.ArgumentParser(
      description="Multi-channel Audio Player with UDP")
  parser.add_argument("--udp_bind_host", type=str, default="127.0.0.1")
  parser.add_argument("--udp_bind_port", type=int, default=7070)
  parser.add_argument("--udp_send_host", type=str, default="127.0.0.1")
  parser.add_argument("--udp_send_port", type=int, default=7072)
  parser.add_argument("--audio_dir",
                      type=str,
                      default=os.path.join(os.getcwd(), "data", "sounds"))
  parser.add_argument("--button_cool_down_s", type=float, default=0.5)
  args = parser.parse_args()

  pygame.mixer.init()

  module_names = [["flux_0"], ["flux_1"], ["flux_2"], ["flux_3"],
                  ["dispatch", "archive"]]
  # Length of outer list determines channels
  pygame.mixer.set_num_channels(len(module_names))
  modules = {}
  udp_sender = UDPModeSender(host=args.udp_send_host, port=args.udp_send_port)

  for idx, names_in_group in enumerate(module_names):
    module_paths = [os.path.join(args.audio_dir, n) for n in names_in_group]
    needs_sender = any(n in ("dispatch", "archive") for n in names_in_group)
    sender = udp_sender.send if needs_sender else None
    module_instance = AudioModule(names_in_group,
                                  module_paths,
                                  idx,
                                  cooldown=args.button_cool_down_s,
                                  udp_sender=sender)
    for name in names_in_group:
      modules[name] = module_instance

  controller = MultiChannelController(args.udp_bind_host, args.udp_bind_port,
                                      modules)
  controller.run()


if __name__ == "__main__":
  main()
