import argparse
import socket
import time
import threading
import logging
import pygame
import os

logging.basicConfig(level=logging.INFO,
                    format="%(asctime)s %(levelname)s: %(message)s")


class AudioPlayerUDP:

  def __init__(self,
               udp_bind_host="127.0.0.1",
               udp_bind_port=7070,
               audio_dir=os.path.join(os.getcwd(), "data", "sounds"),
               button_cool_down_s=0.5,
               udp_send_host="127.0.0.1",
               udp_send_port=7072):
    self.udp_bind_host = udp_bind_host
    self.udp_bind_port = udp_bind_port
    self.audio_dir = audio_dir
    self.button_cool_down_s = button_cool_down_s
    self.last_button_press = time.time()
    self.playing = False
    self.stop_requested = False

    # Setup UDP listener (for commands)
    self.udp_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    self.udp_socket.bind((self.udp_bind_host, self.udp_bind_port))
    logging.info(
        f"UDP server listening on {(self.udp_bind_host, self.udp_bind_port)}")

    # Setup UDP sender (for mode messages)
    self.udp_send_target = (udp_send_host, udp_send_port)
    self.udp_send_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    logging.info(f"UDP sender sending to {self.udp_send_target}")

    pygame.mixer.init()
    self.track_paths = self.load_tracks()

  def load_tracks(self):
    SUPPORTED_FORMATS = ('.wav', '.mp3', '.ogg')
    tracks = {"dispatch": [], "archive": []}
    for folder in tracks.keys():
      folder_path = os.path.join(self.audio_dir, folder)
      if not os.path.isdir(folder_path):
        logging.warning(f"Directory not found: {folder_path}")
        continue
      for track in sorted(os.listdir(folder_path)):
        if track.lower().endswith(SUPPORTED_FORMATS):
          track_path = os.path.join(folder_path, track)
          tracks[folder].append(track_path)
        else:
          logging.warning(f"Skipping unsupported file format: {track}")
      logging.info(f"Loaded {len(tracks[folder])} tracks from {folder}")
    return tracks

  def send_udp_message(self, message: str):
    try:
      self.udp_send_socket.sendto(message.encode(), self.udp_send_target)
      logging.info(f"Sent UDP message to {self.udp_send_target}: {message}")
    except Exception as e:
      logging.error(f"Error sending UDP message: {e}")

  def play_track(self, folder: str, track_number: int):
    if folder not in self.track_paths:
      return
    if track_number >= len(self.track_paths[folder]):
      logging.error(f"Invalid track number {track_number} for folder {folder}")
      return

    self.playing = True
    self.stop_requested = False
    track_path = self.track_paths[folder][track_number]
    logging.info(f"Playing track: {folder}/{track_number} {track_path}")

    # Send "MODE_BUTTON_ON" before playback starts
    self.send_udp_message("MODE_BUTTON_ON")

    pygame.mixer.music.load(track_path)
    pygame.mixer.music.play()
    while pygame.mixer.music.get_busy() and not self.stop_requested:
      time.sleep(0.1)
    pygame.mixer.music.stop()
    self.playing = False

    # Send "MODE_BUTTON_OFF" after playback stops
    self.send_udp_message("MODE_BUTTON_OFF")

  def process_line(self, line: bytes):
    if not line:
      return
    try:
      decoded_line = line.decode().strip()
      parts = decoded_line.split(",")
      if len(parts) != 2:
        logging.error(f"Invalid command format: {decoded_line}")
        return
      folder = parts[0].strip().lower()
      track_number = int(parts[1].strip())
    except Exception as e:
      logging.error(f"Error processing line '{line}': {e}")
      return

    if folder not in self.track_paths:
      logging.warning(f"Ignoring unknown folder: {folder}")
      return

    logging.info(
        f"Received command: folder={folder}, track_number={track_number}")
    if self.playing:
      self.stop_requested = True
      time.sleep(0.2)  # Allow current playback to terminate
    self.check_button(folder, track_number)

  def check_button(self, folder: str, track_number: int):
    if time.time() - self.last_button_press < self.button_cool_down_s:
      return
    self.last_button_press = time.time()
    if not self.playing:
      threading.Thread(target=self.play_track,
                       args=(folder, track_number),
                       daemon=True).start()

  def run(self):
    try:
      while True:
        data, addr = self.udp_socket.recvfrom(1024)
        self.process_line(data)
    except KeyboardInterrupt:
      logging.info("Exiting UDP audio player...")
    finally:
      self.udp_socket.close()
      self.udp_send_socket.close()


def main():
  parser = argparse.ArgumentParser(
      description="Audio Player with UDP Command Reception "
      "and UDP Mode Button Messaging")
  parser.add_argument("--udp_bind_host",
                      type=str,
                      default="127.0.0.1",
                      help="UDP bind host")
  parser.add_argument("--udp_bind_port",
                      type=int,
                      default=7070,
                      help="UDP bind port for receiving commands")
  parser.add_argument("--udp_send_host",
                      type=str,
                      default="127.0.0.1",
                      help="UDP send host for mode messages")
  parser.add_argument("--udp_send_port",
                      type=int,
                      default=7072,
                      help="UDP send port for mode messages")
  parser.add_argument("--audio_dir",
                      type=str,
                      default=os.path.join(os.getcwd(), "data", "sounds"),
                      help="Directory containing audio tracks "
                      "with subfolders 'dispatch' and 'archive'")
  parser.add_argument("--button_cool_down_s",
                      type=float,
                      default=0.5,
                      help="Button cool-down time in seconds")
  args = parser.parse_args()

  player = AudioPlayerUDP(udp_bind_host=args.udp_bind_host,
                          udp_bind_port=args.udp_bind_port,
                          audio_dir=args.audio_dir,
                          button_cool_down_s=args.button_cool_down_s,
                          udp_send_host=args.udp_send_host,
                          udp_send_port=args.udp_send_port)
  player.run()


if __name__ == "__main__":
  main()
