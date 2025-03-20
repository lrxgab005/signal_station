import serial
import time
import threading
import logging
import pygame
import os

logging.basicConfig(level=logging.INFO,
                    format="%(asctime)s %(levelname)s: %(message)s")


class AudioPlayer:

  def __init__(self,
               port='/dev/cu.usbserial-110',
               baudrate=2000000,
               timeout=1,
               audio_dir=os.path.join(os.getcwd(), "data", "sounds"),
               button_cool_down_s=0.5):
    self.ser = serial.Serial(port, baudrate=baudrate, timeout=timeout)
    self.playing = False
    self.track_number = 0
    self.audio_dir = audio_dir
    pygame.mixer.init()
    self.track_paths = self.load_tracks()
    self.button_cool_down_s = button_cool_down_s
    self.last_button_press = time.time()
    self.button_state = 0

  def load_tracks(self):
    tracks = []
    for track in os.listdir(self.audio_dir):
      tracks.append(os.path.join(self.audio_dir, track))
    return tracks

  def send_serial(self, message: str):
    self.ser.write((message + "\n").encode())

  def play_track(self, track_number: int):
    if track_number >= len(self.track_paths):
      logging.error(f"Invalid track number: {track_number}")
      return

    self.playing = True
    self.send_serial("ON")
    logging.info(
        f"Playing track: {track_number} {self.track_paths[track_number]}")
    pygame.mixer.music.load(self.track_paths[track_number])
    pygame.mixer.music.play()
    while pygame.mixer.music.get_busy():
      time.sleep(0.1)

    self.send_serial("OFF")
    self.playing = False

  def process_line(self, line: bytes):
    if not line:
      return

    try:
      values = list(map(int, line.decode().split(",")))
    except ValueError:
      return

    if len(values) < 2:
      logging.error(f"Invalid data. Values len < 2: {values}")
      return

    track_number, button_state = values[:2]

    if track_number != self.track_number:
      self.track_number = track_number
      logging.info(f"Track number: {self.track_number}")

    self.check_button(self.track_number, button_state)

  def check_button(self, track_number: int, button_state: int):
    self.button_state = button_state if button_state else self.button_state

    if time.time() - self.last_button_press < self.button_cool_down_s:
      return

    self.last_button_press = time.time()
    if self.button_state and not self.playing:
      threading.Thread(target=self.play_track,
                       args=(track_number, ),
                       daemon=True).start()
    self.button_state = 0

  def run(self):
    try:
      while True:
        line = self.ser.readline().strip()
        self.process_line(line)
    except KeyboardInterrupt:
      logging.info("Exiting...")
    finally:
      self.ser.close()


if __name__ == "__main__":
  player = AudioPlayer()
  player.run()
