#! /usr/bin/env python3
import paho.mqtt.client as mqtt
import serial
import time
import json

broker = "10.0.0.26"#"192.168.137.141"  # PC IP
mqttc = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
devices = ["esp32-01", "esp32-02"]
last_send_time = None
def on_connect(client, userdata, flags, reason_code, properties):
    print(f"Connected with result code {reason_code}")
    # Subscribing in on_connect() means that if we lose the connection and
    # reconnect then subscriptions will be renewed.
   # client.subscribe("$SYS/#")
    client.subscribe(f"devices/{devices[0]}/echo/time", qos=0)



def on_message(client, userdata, msg):
    global last_send_time
    if "echo/time" in msg.topic and last_send_time:
        latency_ms = (time.time() - last_send_time) * 1000 / 2
        print(f"Latency: {latency_ms:.1f}ms")


mqttc.on_connect = on_connect
mqttc.on_message = on_message


mqttc.connect(broker, 1883, 60)
# Send commands to ESP32s
mqttc.loop_start()

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
            last_send_time = time.time()
            payload = json.dumps({"t_send": last_send_time})
            mqttc.publish(f"devices/{devices[0]}/set/time", payload, qos=0)
        except ValueError:
            print("Please enter valid integer numbers.")
        # write_serial(command + b"\n")

except KeyboardInterrupt:
    print("\nExiting...")
finally:
    mqttc.loop_stop()
    mqttc.disconnect()

