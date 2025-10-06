#include <Arduino.h>
#include "driver/ledc.h"

#define LED_PIN 4

int zero_steer_digital = 135; //This corresponds to about 1.7V Important: This must be the same as the python config
int zero_throttle_digital = 135; //This corresponds to about 1.7V Important: This must be the same as the python config

const uint8_t DAC_STEERING_PIN = 26;
const uint8_t DAC_THROTTLE_PIN = 25;

int commanded_throttle = zero_throttle_digital; // throttle received over serial
int steering = zero_steer_digital;

void pair_with_car() {
  // Dummy function to illustrate the use of delay()
  dacWrite(DAC_STEERING_PIN, zero_steer_digital);
  dacWrite(DAC_THROTTLE_PIN, zero_throttle_digital);
}

void setup() {
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  
  pair_with_car();
}


/**
 * @brief Reads the throttle value from the command string.
 * 
 * The command string is expected to be in the format "t:<value>".
 * Returns the throttle value as an integer, or -1 if the format is incorrect (does not start with "t:").
 * @param cmd The command string to parse. We expect the format to be "t:<value>", where value is 0-255.
 * @return int The parsed throttle value, or -1 if parsing fails (does not start with "t:").
 */

int readThrottle(String cmd){

  if (cmd.startsWith("t:")){
    String val = cmd.substring(2);
    return val.toInt();
  }
  
  return -1;
  
}


int readSteering(String cmd){

  if (cmd.startsWith("s:")){
    String val = cmd.substring(2);
    return val.toInt();
  }
  
  return -1;
}

void loop() {

  // Receiving a command from laptop:
  if (Serial.available()) {

    // Turn led on
    digitalWrite(LED_PIN, HIGH);

    String cmd = Serial.readStringUntil('\n');
    cmd.trim(); // remove any whitespace/newline characters
    Serial.print("ESP: Received command: ");
    Serial.println(cmd);

    int speed = readThrottle(cmd);
    if (speed != -1) {
      commanded_throttle = speed;
      dacWrite(DAC_THROTTLE_PIN, commanded_throttle);
      Serial.print("ESP: Updated throttle to ");
      Serial.println(commanded_throttle);
    }
    int steer = readSteering(cmd);
    if (steer != -1) {
      steering = steer;
      dacWrite(DAC_STEERING_PIN, steering);
      Serial.print("ESP: Updated steering to ");
      Serial.println(steering);
    }
  }
  else {
    // Turn led off
    digitalWrite(LED_PIN, LOW);
  }
}


