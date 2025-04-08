#!/usr/bin/env python3
import argparse
import sys
import gi
import logging

logging.basicConfig(level=logging.INFO,
                    format="%(asctime)s %(levelname)s: %(message)s")

gi.require_version('Gst', '1.0')
gi.require_version('GstRtspServer', '1.0')
from gi.repository import Gst, GstRtspServer, GObject


class RTSPServer(GstRtspServer.RTSPServer):

  def __init__(self):
    super().__init__()
    factory = GstRtspServer.RTSPMediaFactory()
    # Build pipeline for screen capture using avfvideosrc
    pipeline_str = (
        "( avfvideosrc capture-screen=true ! "
        "videoscale ! videoconvert ! video/x-raw,width=640,height=480 ! "
        "videocrop top=50 left=100 right=250 bottom=100 ! "
        "x264enc speed-preset=ultrafast tune=zerolatency ! "
        "rtph264pay name=pay0 pt=96 )")

    factory.set_launch(pipeline_str)
    factory.set_shared(True)
    factory.connect("media-configure", self.on_media_configure)
    self.get_mount_points().add_factory("/stream", factory)

  def on_media_configure(self, factory, media):
    media.connect("prepared", self.on_media_prepared)

  def on_media_prepared(self, media):
    pipeline = media.get_element()
    bus = pipeline.get_bus()
    bus.add_signal_watch()
    bus.connect("message", self.on_bus_message, pipeline)

  def on_bus_message(self, bus, message, pipeline):
    if message.type == Gst.MessageType.ERROR:
      err, debug = message.parse_error()
      logging.info("Error:", err, debug)
    if message.type == Gst.MessageType.EOS:
      pipeline.seek_simple(Gst.Format.TIME,
                           Gst.SeekFlags.FLUSH | Gst.SeekFlags.KEY_UNIT, 0)


def main():
  parser = argparse.ArgumentParser(description="Stream macOS screen via RTSP.")
  parser.add_argument('--port',
                      type=int,
                      default=8554,
                      help="RTSP server port (default: 8554)")
  args = parser.parse_args()

  Gst.init(None)
  server = RTSPServer()
  server.set_service(str(args.port))
  server.attach(None)

  logging.info(
      f"RTSP stream available at: rtsp://<your-ip>:{args.port}/stream")

  logging.info(f"Local: rtsp://localhost:{args.port}/stream")
  loop = GObject.MainLoop()
  try:
    loop.run()
  except KeyboardInterrupt:
    logging.info("Server stopped.")
    sys.exit()


if __name__ == '__main__':
  main()
