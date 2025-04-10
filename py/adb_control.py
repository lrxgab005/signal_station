#!/usr/bin/env python3
import argparse
import subprocess
import time
import socket
import threading
import logging

logging.basicConfig(level=logging.INFO,
                    format="%(asctime)s %(levelname)s: %(message)s")


class ADBScreenController:

  def __init__(self,
               device_ip="192.168.1.100",
               port=5555,
               udp_bind_host="0.0.0.0",
               udp_bind_port=7073,
               cooldown_s=30):
    self.device_ip = device_ip
    self.port = port
    self.udp_bind_host = udp_bind_host
    self.udp_bind_port = udp_bind_port
    self.cooldown_s = cooldown_s
    self.last_activity = time.time()
    self.screen_on = False
    self.udp_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    self.udp_socket.bind((self.udp_bind_host, self.udp_bind_port))

  def adb(self, *args):
    cmd = ["adb"] + list(args)
    return subprocess.run(cmd,
                          stdout=subprocess.PIPE,
                          stderr=subprocess.PIPE,
                          text=True)

  def ensure_adb_connection(self):
    logging.info(f"Connecting to {self.device_ip}:{self.port} via ADB...")
    self.adb("connect", f"{self.device_ip}:{self.port}")
    out = self.adb("devices").stdout
    if self.device_ip not in out:
      logging.error("Device not found in ADB device list.")
      exit(1)
    logging.info("ADB connection established.")

  def turn_screen_on(self):
    if not self.screen_on:
      logging.info("Turning screen ON")
      self.adb("shell", "input", "keyevent", "KEYCODE_WAKEUP")
      self.adb("shell", "input", "keyevent", "KEYCODE_MENU")
      self.screen_on = True

  def turn_screen_off(self):
    if self.screen_on:
      logging.info("Turning screen OFF")
      self.adb("shell", "input", "keyevent", "KEYCODE_SLEEP")
      self.screen_on = False

  def cooldown_loop(self):
    while True:
      if self.screen_on and (time.time() -
                             self.last_activity) > self.cooldown_s:
        self.turn_screen_off()
      time.sleep(1)

  def run(self):
    self.ensure_adb_connection()
    threading.Thread(target=self.cooldown_loop, daemon=True).start()
    logging.info(
        f"Listening for UDP on {self.udp_bind_host}:{self.udp_bind_port}...")
    try:
      while True:
        data, _ = self.udp_socket.recvfrom(1024)
        if data:
          self.last_activity = time.time()
          self.turn_screen_on()
    except KeyboardInterrupt:
      logging.info("Shutting down...")
    finally:
      self.udp_socket.close()


def main():
  parser = argparse.ArgumentParser(description="ADB UDP Screen Controller")
  parser.add_argument("--device_ip",
                      default="192.168.1.100",
                      help="ADB device IP")
  parser.add_argument("--port", default=5555, type=int, help="ADB TCP port")
  parser.add_argument("--udp_bind_host",
                      default="0.0.0.0",
                      help="UDP bind host")
  parser.add_argument("--udp_bind_port",
                      default=7073,
                      type=int,
                      help="UDP bind port")
  parser.add_argument("--cooldown_s",
                      default=30,
                      type=int,
                      help="Seconds after last UDP msg to turn screen off")
  args = parser.parse_args()

  controller = ADBScreenController(device_ip=args.device_ip,
                                   port=args.port,
                                   udp_bind_host=args.udp_bind_host,
                                   udp_bind_port=args.udp_bind_port,
                                   cooldown_s=args.cooldown_s)
  controller.run()


if __name__ == "__main__":
  main()
