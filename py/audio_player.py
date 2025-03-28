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


class AudioModule:

  def __init__(self, name, track_dir, channel_id, cooldown=0.5):
    self.name = name
    self.track_dir = track_dir
    self.channel_id = channel_id
    self.cooldown = cooldown
    self.last_command_time = 0
    self.sounds = self.load_sounds()
    # assign a dedicated mixer channel
    self.channel = pygame.mixer.Channel(channel_id)
    logging.info(f"Initialized module '{name}' on channel {channel_id}")

  def load_sounds(self):
    sounds = []
    if not os.path.isdir(self.track_dir):
      logging.warning(f"Directory not found: {self.track_dir}")
      return sounds
    for fname in sorted(os.listdir(self.track_dir)):
      if fname.lower().endswith(SUPPORTED_FORMATS):
        path = os.path.join(self.track_dir, fname)
        try:
          sound = pygame.mixer.Sound(path)
          sounds.append(sound)
          logging.info(f"{self.name}: Loaded sound {path}")
        except Exception as e:
          logging.error(f"{self.name}: Error loading {path}: {e}")
      else:
        logging.warning(f"{self.name}: Skipping unsupported file {fname}")
    return sounds

  def set_volume(self, volume_percent):
    # volume: 0-100 -> scale to 0.0-1.0
    vol = max(0.0, min(1.0, volume_percent / 100.0))
    self.channel.set_volume(vol)
    logging.info(f"{self.name}: Volume set to {vol}")

  def play_track(self, track_index, loop=False):
    if track_index < 0 or track_index >= len(self.sounds):
      logging.error(f"{self.name}: Invalid track index {track_index}")
      return
    now = time.time()
    if now - self.last_command_time < self.cooldown:
      logging.info(f"{self.name}: Command ignored due to cooldown")
      return
    self.last_command_time = now
    sound = self.sounds[track_index]
    self.channel.play(sound, loops=-1 if loop else 0)
    logging.info(f"{self.name}: Playing track {track_index}")

  def process_command(self, command, value):
    try:
      if command == "volume":
        self.set_volume(int(value))
      elif command == "play":
        self.play_track(int(value))
      elif command == "loop":
        self.play_track(int(value), loop=True)
      elif command == "stop":
        self.channel.stop()
        logging.info(f"{self.name}: Stopped playback")
      else:
        logging.warning(f"{self.name}: Unknown command '{command}'")
    except Exception as e:
      logging.error(
          f"{self.name}: Error processing command '{command}' with value '{value}': {e}"
      )


class MultiChannelController:

  def __init__(self, udp_bind_host, udp_bind_port, modules):
    self.udp_bind_host = udp_bind_host
    self.udp_bind_port = udp_bind_port
    self.modules = modules  # dict mapping module names to AudioModule
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
        logging.error(f"Invalid message format: {message}")
        return
      module_name, command, value = parts
      if module_name in self.modules:
        self.modules[module_name].process_command(command, value)
      else:
        logging.warning(f"Unknown module: {module_name}")
    except Exception as e:
      logging.error(f"Error processing message: {e}")


def main():
  parser = argparse.ArgumentParser(
      description="Multi-channel Audio Player via UDP")
  parser.add_argument("--udp_bind_host", type=str, default="127.0.0.1")
  parser.add_argument("--udp_bind_port", type=int, default=7070)
  parser.add_argument("--audio_dir",
                      type=str,
                      default=os.path.join(os.getcwd(), "data", "sounds"))
  args = parser.parse_args()

  pygame.mixer.init()
  module_names = [
      "flux_0", "flux_1", "flux_2", "flux_3", "dispatch", "archive"
  ]
  pygame.mixer.set_num_channels(len(module_names))
  modules = {}
  for idx, name in enumerate(module_names):
    module_path = os.path.join(args.audio_dir, name)
    modules[name] = AudioModule(name, module_path, idx)

  controller = MultiChannelController(args.udp_bind_host, args.udp_bind_port,
                                      modules)
  controller.run()


if __name__ == "__main__":
  main()
