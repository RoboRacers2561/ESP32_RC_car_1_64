import serial
import time
from pynput import keyboard

# -----------------------------
# Serial setup
# -----------------------------
SERIAL_PORT = "/dev/tty.usbserial-140"
SERIAL_BAUD = 115200
port = serial.Serial(SERIAL_PORT, SERIAL_BAUD, timeout=1)

# -----------------------------
# Throttle/Steering setup
# -----------------------------
throttle = 0
steering = 0
THROTTLE_STEP = 50    # How fast throttle ramps up/down
STEERING_STEP = 50    # How fast steering ramps
MAX_VAL = 1000        # Maximum magnitude for throttle/steering

keys_pressed = set()  # Track currently held keys
last_throttle = None
last_steering = None
sent_config = False


def send_config(port, throttle_zero=900, steering_zero=1400):
    port.write(f"CONFIG,{throttle_zero},{steering_zero}\n".encode())
    # Optionally wait a short time for ESP32 to parse
    time.sleep(1)


# -----------------------------
# Key press/release handlers
# -----------------------------
def on_press(key):
    try:
        keys_pressed.add(key.char)
    except AttributeError:
        pass  # handle special keys if needed

def on_release(key):
    try:
        keys_pressed.discard(key.char)
    except AttributeError:
        pass

listener = keyboard.Listener(on_press=on_press, on_release=on_release)
listener.start()

# -----------------------------
# Main control loop
# -----------------------------
try:
    while True:

        # if not sent_config:
        #     send_config(port)
        #     sent_config = True

        # Throttle control (W/S keys)
        if 'w' in keys_pressed:
            throttle = min(throttle - THROTTLE_STEP, MAX_VAL)
        elif 's' in keys_pressed:
            throttle = max(throttle + THROTTLE_STEP, -MAX_VAL)
        else:
            # Slowly return to neutral
            if throttle > 0:
                throttle -= THROTTLE_STEP
            elif throttle < 0:
                throttle += THROTTLE_STEP

        # Steering control (A/D keys)
        if 'a' in keys_pressed:
            steering = max(steering + STEERING_STEP, -MAX_VAL)
        elif 'd' in keys_pressed:
            steering = min(steering - STEERING_STEP, MAX_VAL)
        else:
            # Slowly return to center
            if steering > 0:
                steering -= STEERING_STEP
            elif steering < 0:
                steering += STEERING_STEP

        # Send command to ESP32
        if throttle != last_throttle or steering != last_steering:
            cmd = f"{throttle},{steering}\n".encode()
            port.write(cmd)
            last_throttle = throttle
            last_steering = steering
        
        if port.in_waiting:
            line = port.readline().decode(errors='ignore').strip()
            print(line, end='\r')  # or process it

        

        # Optional: print status
        #print(f"Throttle: {throttle} | Steering: {steering}", end="\r")

        time.sleep(0.05)

except KeyboardInterrupt:
    print("\nController stopped")
    port.close()

