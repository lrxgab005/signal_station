#!/usr/bin/env python3
from typing import Any
import argparse
import sys
import gi

gi.require_version('Gst', '1.0')
gi.require_version('GstRtspServer', '1.0')
from gi.repository import Gst, GstRtspServer, GObject


class RTSPServer(GstRtspServer.RTSPServer):

  def __init__(self, filepath):
    super().__init__()
    self.filepath = filepath
    factory = GstRtspServer.RTSPMediaFactory()
    # Build a pipeline for file playback, encoding, and RTSP streaming.
    pipeline_str = (f"( filesrc location={filepath} ! decodebin "
                    " ! x264enc speed-preset=ultrafast tune=zerolatency "
                    " ! rtph264pay name=pay0 pt=96 )")
    factory.set_launch(pipeline_str)
    factory.set_shared(True)
    # Connect to configure the media for looping.
    factory.connect("media-configure", self.on_media_configure)
    self.get_mount_points().add_factory("/stream", factory)

  def on_media_configure(self, _factory: Any, media: Any) -> None:
    media.connect("prepared", self.on_media_prepared)

  def on_media_prepared(self, media: Any) -> None:
    pipeline = media.get_element()
    bus = pipeline.get_bus()
    bus.add_signal_watch()
    bus.connect("message", self.on_bus_message, pipeline)

  def on_bus_message(self, _bus: Any, message: Any, pipeline: Any) -> None:
    if message.type == Gst.MessageType.EOS:
      # On end-of-stream, seek back to the start.
      pipeline.seek_simple(Gst.Format.TIME,
                           Gst.SeekFlags.FLUSH | Gst.SeekFlags.KEY_UNIT, 0)


def main():
  parser = argparse.ArgumentParser(
      description="Stream a local video file over RTSP in loop.")
  parser.add_argument('file', help="Path to the local video file")
  parser.add_argument('--port',
                      type=int,
                      default=8554,
                      help="RTSP server port (default: 8554)")
  args = parser.parse_args()

  Gst.init(None)
  server = RTSPServer(args.file)
  server.set_service(str(args.port))
  server.attach(None)

  print(f"RTSP stream available at: rtsp://<your-ip>:{args.port}/stream")
  loop = GObject.MainLoop()
  try:
    loop.run()
  except KeyboardInterrupt:
    print("Server stopped.")
    sys.exit()


if __name__ == '__main__':
  main()
