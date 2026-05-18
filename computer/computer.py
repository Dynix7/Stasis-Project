import serial
import serial.tools.list_ports
import time
from pynput import keyboard

def find_pico():
    for port in serial.tools.list_ports.comports():
        if "2E8A" in port.hwid.upper():
            return port.device
    raise Exception("Pico not found. Is it plugged in?")

def send(ser, message):
    ser.write((message + '\n').encode('utf-8'))
    print(f"Sent: {message}")

def on_press(key):
    try:
        if key.char in ('a', 'b', 'c'):
            send(ser, key.char)
            print("Key sent :)")
    except AttributeError:
        pass  # ignore special keys

try:
    port = find_pico()
    ser = serial.Serial(port, 115200, timeout=2)
    time.sleep(2)
    ser.reset_input_buffer()
    print(f"Connected on {port} — press pedals (Ctrl+C to quit)")

    with keyboard.Listener(on_press=on_press) as listener:
        listener.join()

except KeyboardInterrupt:
    print("Stopped.")
except Exception as e:
    print(f"Error: {e}")
finally:
    if 'ser' in locals() and ser.is_open:
        ser.close()
