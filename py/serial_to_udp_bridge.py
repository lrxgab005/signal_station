import argparse
import serial
import serial.tools.list_ports
import socket
import time
import logging
import threading

logging.basicConfig(level=logging.INFO,
                    format="%(asctime)s %(levelname)s: %(message)s")


def udp_receiver(ser, udp_send_targets, udp_listen_targets):
  udp_rx_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
  for listen_target in udp_listen_targets:
    host, port = listen_target.split(":")
    udp_rx_socket.bind((host, int(port)))
    logging.info(
        f"Listening for UDP on port {port} to forward to serial and broadcast")

  udp_tx_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

  while True:
    try:
      data, addr = udp_rx_socket.recvfrom(1024)
      logging.info(f"UDP from {addr}: {data}")
      ser.write(data + b'\n')
      for host, port in udp_send_targets:
        udp_tx_socket.sendto(data, (host, port))
    except Exception as e:
      logging.error(f"UDP receive error: {e}")
      break


def main():
  parser = argparse.ArgumentParser(
      description="Serial to UDP bridge (multi-target)")
  parser.add_argument("--serial_port",
                      type=str,
                      required=True,
                      help="Serial port (auto-detect USB if not provided)")
  parser.add_argument("--baudrate",
                      type=int,
                      default=9700,
                      help="Baud rate for serial connection")
  parser.add_argument("--timeout",
                      type=float,
                      default=2,
                      help="Timeout for serial connection")
  parser.add_argument("--udp_send_targets",
                      nargs="+",
                      default=["127.0.0.1:7070", "127.0.0.1:7071"],
                      help="List of UDP send targets in host:port.")
  parser.add_argument("--udp_listen_targets",
                      nargs="+",
                      default=["127.0.0.1:7072"],
                      help="List of UDP receive targets in host:port.")

  args = parser.parse_args()

  udp_send_targets = []
  for t in args.udp_send_targets:
    try:
      host, port = t.split(":")
      udp_send_targets.append((host, int(port)))
    except Exception as e:
      logging.error(f"Invalid target format: {t}, {e}")
      return

  ser = serial.Serial(args.serial_port,
                      baudrate=args.baudrate,
                      timeout=args.timeout)
  udp_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

  logging.info(f"Forwarding serial to UDP targets: {udp_send_targets}")

  # Start UDP receiver thread with broadcast support
  udp_rx_thread = threading.Thread(target=udp_receiver,
                                   args=(ser, udp_send_targets,
                                         args.udp_listen_targets),
                                   daemon=True)
  udp_rx_thread.start()

  # Forward serial to UDP targets
  try:
    while True:
      line = ser.readline().strip()
      if line:
        logging.info(f"Received from serial: {line}")
        for host, port in udp_send_targets:
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
