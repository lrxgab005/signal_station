import serial

# Open the serial port
ser = serial.Serial('/dev/cu.usbserial-12120', baudrate=9600, timeout=1)

try:
  while True:
    line = ser.readline().strip()  # Read a line, remove trailing newlines
    if line:
      try:
        # Convert bytes to a list of integers
        values = list(map(int, line.decode().split(",")))
        print(",".join(map(str, values)))
      except ValueError:
        print("Invalid data:", line.decode())
except KeyboardInterrupt:
  print("\nExiting...")
finally:
  ser.close()
