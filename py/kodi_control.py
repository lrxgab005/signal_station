#!/usr/bin/env python3
import argparse
import json
import sys
import requests


class KodiController:
  """
  A simple Kodi JSON-RPC controller to play a file from a directory on a remote device.
  """

  def __init__(self,
               ip="192.168.1.76",
               port="8080",
               user="kodi",
               password="kodi",
               directory=None):
    self.url = f"http://{ip}:{port}/jsonrpc"
    self.auth = (user, password)

    if directory is None:
      self.directory = self.get_first_video_source()
    else:
      self.directory = directory
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
    sources = result.get("result", {}).get("sources", [])
    return sources

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

  def list_video_sources(self):
    sources = self.get_video_sources()
    if not sources:
      print("No video sources available.")
      return
    print("Available Video Sources:")
    for idx, source in enumerate(sources):
      label = source.get("label", source.get("file", "Unknown"))
      file_path = source.get("file", "Unknown")
      print(f"[{idx}] {label} -> {file_path}")

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
      print(
          f"Index {index} out of range. Only {len(self.files)} file(s) available."
      )
      sys.exit(1)
    file_path = file_entry.get("file")
    if not file_path:
      print("File entry does not contain a file path. Exiting.")
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


def main():
  parser = argparse.ArgumentParser(
      description="Kodi JSON-RPC Controller - Always plays a file.")
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
  parser.add_argument("--dir",
                      default=None,
                      help="Directory to load files from "
                      "(if omitted, the first video source is used)")
  parser.add_argument("--index",
                      type=int,
                      default=0,
                      help="Index of the file to play (default: 0)")
  parser.add_argument("--list-dirs",
                      action="store_true",
                      help="List available video sources and exit")

  args = parser.parse_args()

  controller = KodiController(ip=args.ip,
                              port=args.port,
                              user=args.user,
                              password=args.password,
                              directory=args.dir)

  if args.list_dirs:
    controller.list_video_sources()
  else:
    controller.play_by_index(args.index)


if __name__ == "__main__":
  main()
