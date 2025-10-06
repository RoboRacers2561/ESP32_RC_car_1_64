#! /usr/bin/env python3
import paho.mqtt.client as mqtt
import serial
import time

broker = "10.0.0.73"  # PC IP
mqttc = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)

def on_connect(client, userdata, flags, reason_code, properties):
    print(f"Connected with result code {reason_code}")
    # Subscribing in on_connect() means that if we lose the connection and
    # reconnect then subscriptions will be renewed.
    client.subscribe("$SYS/#")

def on_message(client, userdata, msg):
    print(msg.topic+" "+str(msg.payload))

mqttc.on_connect = on_connect
mqttc.on_message = on_message


mqttc.connect(broker, 1883, 60)
# Send commands to ESP32s
devices = ["esp32-01", "esp32-02"]
# for d in devices:
#     mqttc.subscribe(f"devices/{d}/set")
#     mqttc.publish(f"devices/{d}/set", "LED_ON")

try:
    while (True):
        user_input = input("Enter throttle,steering values (ex: 50,-10): ")

        if "," not in user_input:
            print("Invalid input: {}".format(user_input))
            continue
        throttle_str, steering_str = user_input.split(",")
        try:
            throttle = int(throttle_str)
            steering = int(steering_str)
            print(f"Throttle: {throttle}, Steering: {steering}")
            mqttc.publish(f"devices/{devices[0]}/set/throttle", str(throttle))
            mqttc.publish(f"devices/{devices[0]}/set/steering", str(steering))
        except ValueError:
            print("Please enter valid integer numbers.")
        # write_serial(command + b"\n")

except KeyboardInterrupt:
    print("\nExiting...")
finally:
    mqttc.loop_stop()
    mqttc.disconnect()