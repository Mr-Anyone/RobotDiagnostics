import serial

# Open the serial port
ser = serial.Serial('COM3', 115200)

# Read one byte
byte_data = ser.read(1)  # reads exactly 1 byte

# byte_data is of type bytes, so you may want to convert it
if byte_data:
    print(f"Received byte: {byte_data[0]}")  # as integer
    print(f"Received byte (hex): {byte_data.hex()}")  # as hex

ser.close()