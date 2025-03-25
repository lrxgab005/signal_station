import argparse
import serial
import serial.tools.list_ports
import socket
import time
import logging

logging.basicConfig(level=logging.INFO,
                    format="%(asctime)s %(levelname)s: %(message)s")


def main():
  parser = argparse.ArgumentParser(
      description="Serial to UDP bridge (multi-target)")
  parser.add_argument("--port",
                      type=str,
                      default=None,
                      help="Serial port (auto-detect USB if not provided)")
  parser.add_argument("--baudrate",
                      type=int,
                      default=9700,
                      help="Baud rate for serial connection")
  parser.add_argument("--timeout",
                      type=float,
                      default=2,
                      help="Timeout for serial connection")
  parser.add_argument("--udp_targets",
                      nargs="+",
                      default=["127.0.0.1:7070", "127.0.0.1:7071"],
                      help="List of UDP targets in host:port."
                      "Default (e.g., 127.0.0.1:7070 127.0.0.1:7071)")
  args = parser.parse_args()

  if args.port is None:
    ports = list(serial.tools.list_ports.comports())
    usb_ports = [p for p in ports if "USB" in p.device.upper()]
    if usb_ports:
      args.port = usb_ports[0].device
      logging.info(f"Auto-detected USB serial port: {args.port}")
    elif ports:
      args.port = ports[0].device
      logging.info(f"Auto-detected serial port (no 'USB' found): {args.port}")
    else:
      logging.error("No serial ports found.")
      return

  udp_targets = []
  for t in args.udp_targets:
    try:
      host, port = t.split(":")
      udp_targets.append((host, int(port)))
    except Exception as e:
      logging.error(f"Invalid target format: {t}, {e}")
      return

  ser = serial.Serial(args.port, baudrate=args.baudrate, timeout=args.timeout)
  udp_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

  logging.info(f"Forwarding serial to UDP targets: {udp_targets}")

  try:
    while True:
      line = ser.readline().strip()
      if line:
        logging.info(f"Received: {line}")
        for host, port in udp_targets:
          udp_socket.sendto(line, (host, port))
      else:
        time.sleep(0.01)
  except KeyboardInterrupt:
    logging.info("Exiting...")
  finally:
    ser.close()
    udp_socket.close()


if __name__ == "__main__":
  main()
