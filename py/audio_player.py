import serial
import time
import threading
import logging

logging.basicConfig(level=logging.INFO,
                    format="%(asctime)s %(levelname)s: %(message)s")


class AudioPlayer:

  def __init__(self, port='/dev/cu.usbserial-12120', baudrate=9600, timeout=1):
    self.ser = serial.Serial(port, baudrate=baudrate, timeout=timeout)
    self.playing = False
    self.track_number = 0

  def send_serial(self, message: str):
    self.ser.write((message + "\n").encode())

  def play_track(self, track_number: int):
    self.playing = True
    self.send_serial("ON")
    logging.info(f"Playing track: {track_number}")
    # Simulated playback; replace with actual audio code.
    time.sleep(5)
    self.send_serial("OFF")
    self.playing = False

  def process_line(self, line: bytes):
    if not line:
      return

    try:
      values = list(map(int, line.decode().split(",")))
    except ValueError:
      logging.info("Invalid data:", line.decode())
      return

    if len(values) < 2:
      return

    track_number, button_state = values[:2]

    if track_number != self.track_number:
      self.track_number = track_number
      logging.info(f"Track number: {self.track_number}")

    self.check_button(self.track_number, button_state)

  def check_button(self, track_number: int, button_state: int):
    if button_state and not self.playing:
      threading.Thread(target=self.play_track,
                       args=(track_number, ),
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


if __name__ == "__main__":
  player = AudioPlayer()
  player.run()
