#!/usr/bin/env python3
import argparse
import json
import sys
import requests
import time
import socket


class KodiController:

  def __init__(self,
               ip="192.168.1.76",
               port="8080",
               user="kodi",
               password="kodi",
               directory=None):
    self.url = f"http://{ip}:{port}/jsonrpc"
    self.auth = (user, password)
    # Auto-detect video source if not provided.
    self.directory = directory if directory is not None else self.get_first_video_source(
    )
    self.files = self.load_files(self.directory)

  def json_rpc_request(self, payload):
    headers = {'Content-Type': 'application/json'}
    try:
      response = requests.post(self.url,
                               headers=headers,
                               data=json.dumps(payload),
                               auth=self.auth)
      response.raise_for_status()
      return response.json()
    except requests.exceptions.RequestException as e:
      print(f"Request error: {e}")
      sys.exit(1)

  def get_video_sources(self):
    payload = {
        "jsonrpc": "2.0",
        "method": "Files.GetSources",
        "params": {
            "media": "video"
        },
        "id": 1
    }
    result = self.json_rpc_request(payload)
    return result.get("result", {}).get("sources", [])

  def get_first_video_source(self):
    sources = self.get_video_sources()
    if not sources:
      print("No video sources found. Exiting.")
      sys.exit(1)
    first_source = sources[0]
    directory = first_source.get("file")
    if not directory:
      print("The first video source has no directory. Exiting.")
      sys.exit(1)
    print(f"Using video source: {first_source.get('label', directory)}")
    return directory

  def load_files(self, directory):
    payload = {
        "jsonrpc": "2.0",
        "method": "Files.GetDirectory",
        "params": {
            "directory": directory,
            "media": "video"
        },
        "id": 1
    }
    result = self.json_rpc_request(payload)
    files = result.get("result", {}).get("files", [])
    if not files:
      print(f"No files found in directory: {directory}")
      sys.exit(1)
    return files

  def play_by_index(self, index):
    try:
      file_entry = self.files[index]
    except IndexError:
      print(f"Index {index} out of range. Only {len(self.files)} available.")
      return
    file_path = file_entry.get("file")
    if not file_path:
      print("File entry is missing file path. Exiting.")
      sys.exit(1)
    payload = {
        "jsonrpc": "2.0",
        "method": "Player.Open",
        "params": {
            "item": {
                "file": file_path
            }
        },
        "id": 1
    }
    self.json_rpc_request(payload)
    print(f"Playing file: {file_path}")


class UDPKodiController(KodiController):

  def __init__(self,
               ip="192.168.1.76",
               port="8080",
               user="kodi",
               password="kodi",
               directory=None,
               udp_bind_host="127.0.0.1",
               udp_bind_port=7071,
               button_cool_down_s=0.5):
    super().__init__(ip=ip,
                     port=port,
                     user=user,
                     password=password,
                     directory=directory)
    self.udp_bind_host = udp_bind_host
    self.udp_bind_port = udp_bind_port
    self.button_cool_down_s = button_cool_down_s
    self.last_button_press = time.time()
    self.udp_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    self.udp_socket.bind((self.udp_bind_host, self.udp_bind_port))
    print(
        f"UDP server listening on {(self.udp_bind_host, self.udp_bind_port)}")

  def process_message(self, data):
    try:
      decoded = data.decode().strip()
      label, index = decoded.split(",")
      if label.strip().lower() != "video":
        print(f"Ignoring non-video message: {decoded}")
        return
      index = int(index.strip())
    except Exception as e:
      print(f"Error parsing UDP data: {data} ({e})")
      return

    if time.time() - self.last_button_press < self.button_cool_down_s:
      return
    self.last_button_press = time.time()
    if index >= len(self.files):
      print(f"Index {index} out of range. Only {len(self.files)} available.")
      return
    self.play_by_index(index)

  def run(self):
    try:
      while True:
        data, addr = self.udp_socket.recvfrom(1024)
        if data:
          self.process_message(data)
    except KeyboardInterrupt:
      print("Exiting UDP Kodi controller...")
    finally:
      self.udp_socket.close()


def main():
  parser = argparse.ArgumentParser(
      description="Kodi JSON-RPC Controller - UDP Mode Only")
  parser.add_argument("--ip",
                      default="192.168.1.76",
                      help="Kodi IP address (default: 192.168.1.76)")
  parser.add_argument("--port",
                      default="8080",
                      help="Kodi port (default: 8080)")
  parser.add_argument("--user",
                      default="kodi",
                      help="Kodi username (default: kodi)")
  parser.add_argument("--password",
                      default="kodi",
                      help="Kodi password (default: kodi)")
  parser.add_argument(
      "--dir",
      default=None,
      help="Directory for video files (default: first video source)")
  parser.add_argument("--udp_bind_host",
                      default="127.0.0.1",
                      help="UDP bind host (default: 127.0.0.1)")
  parser.add_argument("--udp_bind_port",
                      type=int,
                      default=7071,
                      help="UDP bind port (default: 7071)")
  parser.add_argument("--button_cool_down_s",
                      type=float,
                      default=0.5,
                      help="Button cool-down time (seconds)")
  args = parser.parse_args()

  controller = UDPKodiController(ip=args.ip,
                                 port=args.port,
                                 user=args.user,
                                 password=args.password,
                                 directory=args.dir,
                                 udp_bind_host=args.udp_bind_host,
                                 udp_bind_port=args.udp_bind_port,
                                 button_cool_down_s=args.button_cool_down_s)
  controller.run()


if __name__ == "__main__":
  main()
