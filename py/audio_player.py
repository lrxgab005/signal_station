import argparse
import serial
import serial.tools.list_ports
import time
import threading
import logging
import pygame
import os

logging.basicConfig(level=logging.INFO,
                    format="%(asctime)s %(levelname)s: %(message)s")


class AudioPlayer:

  def __init__(self,
               port=None,
               baudrate=2000000,
               timeout=1,
               audio_dir=os.path.join(os.getcwd(), "data", "sounds"),
               button_cool_down_s=0.5):
    if port is None:
      ports = list(serial.tools.list_ports.comports())
      usb_ports = [p for p in ports if "USB" in p.device.upper()]
      if usb_ports:
        port = usb_ports[0].device
        logging.info(f"Auto-detected USB serial port: {port}")
      elif ports:
        port = ports[0].device
        logging.info(f"Auto-detected serial port (no 'USB' found): {port}")
      else:
        raise Exception("No serial ports found.")

    self.ser = serial.Serial(port, baudrate=baudrate, timeout=timeout)
    self.playing = False
    self.audio_dir = audio_dir
    pygame.mixer.init()
    self.track_paths = self.load_tracks(
    )  # dict with keys 'dispatch' and 'archive'
    self.button_cool_down_s = button_cool_down_s
    self.last_button_press = time.time()
    self.stop_requested = False

    self.send_serial("ON")

  def load_tracks(self):
    # Common audio formats supported by pygame
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

  def send_serial(self, message: str):
    self.ser.write((message + "\n").encode())

  def play_track(self, folder: str, track_number: int):
    if folder not in self.track_paths:
      logging.error(f"Invalid folder: {folder}")
      return
    if track_number >= len(self.track_paths[folder]):
      logging.error(f"Invalid track number {track_number} for folder {folder}")
      return

    self.playing = True
    self.stop_requested = False
    self.send_serial("ON")
    track_path = self.track_paths[folder][track_number]
    logging.info(f"Playing track: {folder}/{track_number} {track_path}")
    pygame.mixer.music.load(track_path)
    pygame.mixer.music.play()
    while pygame.mixer.music.get_busy() and not self.stop_requested:
      time.sleep(0.1)
    pygame.mixer.music.stop()
    self.send_serial("OFF")
    self.playing = False

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

    logging.info(
        f"Received command: folder={folder}, track_number={track_number}")
    if self.playing:
      self.stop_requested = True
      time.sleep(0.2)  # Give time for current playback to stop
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
        line = self.ser.readline().strip()
        self.process_line(line)
    except KeyboardInterrupt:
      logging.info("Exiting...")
    finally:
      self.ser.close()


def main():
  parser = argparse.ArgumentParser(
      description="Audio Player with Serial USB Detection")
  parser.add_argument(
      "--port",
      type=str,
      default=None,
      help="Serial port path (auto-detect USB if not provided)")
  parser.add_argument("--baudrate",
                      type=int,
                      default=2000000,
                      help="Baud rate for the serial connection")
  parser.add_argument("--timeout",
                      type=float,
                      default=1,
                      help="Timeout for the serial connection")
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

  player = AudioPlayer(port=args.port,
                       baudrate=args.baudrate,
                       timeout=args.timeout,
                       audio_dir=args.audio_dir,
                       button_cool_down_s=args.button_cool_down_s)
  player.run()


if __name__ == "__main__":
  main()
