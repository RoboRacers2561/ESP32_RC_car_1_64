#include <Arduino.h>
#include "driver/ledc.h"

#define LED_PIN 4

#define PWM_FREQ_HZ 10000
#define PWM_BIT_DEPTH 12

#define PWM_CHNL_THROTTLE 2
#define PWM_PIN_THROTTLE 10

#define PWM_CHNL_STEERING 3
#define PWM_PIN_STEERING 9

#define INCREMENT 100

#define MAX_PWM 3200
#define MIN_PWM 0

#define MAX_BUFF_LEN 4

#define WAIT_TIME_ms 2000 //How long to wait for the RC controller to connect

#define MIN_THROTTLE_FROM_STOP 4
#define START_THROTTLE 6
#define START_THROTTLE_TIME_ms 100

#define DEBUG Serial1

// Need to use two throttle parameters as the static friction prevents
// the car from driving slow enough to be controllable on a small track

int zero_pwm_steer = 1400; //This corresponds to about 1.7V
int zero_pwm_throttle = 1050; //This corresponds to about 1.7V

int commanded_throttle = zero_pwm_throttle; // throttle received over serial
int applied_throttle = zero_pwm_throttle; // thrtottle sent to the motors
int prev_throttle = zero_pwm_throttle;
bool jump_start = false;

int steering = zero_pwm_steer;
int prev_steering = zero_pwm_steer;
bool stopped = false;

int state = 1;
int iteration = 0;

uint8_t cmd_buffer[MAX_BUFF_LEN];
uint8_t character;
uint8_t i = 0;
uint32_t cmd_strength = 0;

unsigned long start_time_ms = 0;
unsigned long current_time_ms = 0;
unsigned long prev_time_ms = 0;


void checkConfig() {
  while (Serial.available()) {
    String line = Serial.readStringUntil('\n');
    line.trim();

    if (line.startsWith("CONFIG")) {
      int firstComma = line.indexOf(',');
      int secondComma = line.indexOf(',', firstComma + 1);

      if (firstComma == -1 || secondComma == -1) {
        Serial.println("CONFIG format error");
        return;
      }

      int throttle_zero = line.substring(firstComma + 1, secondComma).toInt();
      int steer_zero    = line.substring(secondComma + 1).toInt();

      zero_pwm_throttle = throttle_zero;
      zero_pwm_steer = steer_zero;

      Serial.print("Config updated: ");
      Serial.println(line);
    }
  }
}


void setup() {
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  // Set pwmchannel to use,  frequency in Hz, number of bits)
  ledcSetup(PWM_CHNL_THROTTLE, PWM_FREQ_HZ, PWM_BIT_DEPTH);
  ledcSetup(PWM_CHNL_STEERING, PWM_FREQ_HZ, PWM_BIT_DEPTH);
  ledcAttachPin(PWM_PIN_THROTTLE, PWM_CHNL_THROTTLE);
  ledcAttachPin(PWM_PIN_STEERING, PWM_CHNL_STEERING);

  start_time_ms = millis();
}

void loop() {

  // Receiving a command from laptop:
  if (Serial.available()) {
  String cmd = Serial.readStringUntil('\n');
  int comma = cmd.indexOf(',');
  if (comma > 0) {
    int t = cmd.substring(0, comma).toInt();   // -1000 .. 1000
    int s = cmd.substring(comma + 1).toInt();  // -1000 .. 1000

    commanded_throttle = zero_pwm_throttle + t; // map around neutral
    steering = zero_pwm_steer + s;

    // constrain to valid PWM range
    commanded_throttle = constrain(commanded_throttle, MIN_PWM, MAX_PWM);
    steering = constrain(steering, MIN_PWM, MAX_PWM);
    Serial.print("ESP: Driving. Throttle: ");
    Serial.print(commanded_throttle);
    Serial.print("Steering:");
    Serial.println(steering);
  }
}

  // state 0: wait to establish connection with RC controller
  if (state == 1){
    // pot read max value: 4096 (12bit)
    commanded_throttle = zero_pwm_throttle;
    applied_throttle = zero_pwm_throttle;
    steering = zero_pwm_steer;

    // writes a duty cycle to the specified pwmchannel 
    // (which in this case was linked to pin 4)
    ledcWrite(PWM_CHNL_THROTTLE, applied_throttle);
    ledcWrite(PWM_CHNL_STEERING, steering);

    // Wait 2 seconds for RC controller to connect:
    current_time_ms = millis();
    if ((current_time_ms - start_time_ms) > WAIT_TIME_ms){
      state = 2;
    }
  }

  // state 2: drive
  if (state == 2){
    applied_throttle = commanded_throttle;
    
    // For throttle control if the car is stopped we will accelerate above the
    // static friction threshold and then if necessary dial down the applied 
    // throttle to the commanded value:
    if ((prev_throttle == 0) && 
        (applied_throttle < MIN_THROTTLE_FROM_STOP) &&
        (jump_start == false)){
      // Accelerate above static friction for 100ms
      applied_throttle = START_THROTTLE;
      prev_time_ms = millis();
      jump_start = true;
    }

    // Check if we need to throttle-down:
    if (jump_start == true){
      current_time_ms = millis();
      if (current_time_ms - prev_time_ms > START_THROTTLE_TIME_ms){
        applied_throttle = commanded_throttle;
        jump_start = false;
      }
    }

    //Sanitize controls before sending them out to motors:
    applied_throttle = (applied_throttle <= MIN_PWM) ? MIN_PWM : applied_throttle;
    applied_throttle = (applied_throttle >= MAX_PWM) ? MAX_PWM : applied_throttle;
    steering = (steering <= MIN_PWM) ? MIN_PWM : steering;
    steering = (steering >= MAX_PWM) ? MAX_PWM : steering;

    // Send commands only when value change:
    if (prev_throttle != applied_throttle) {
      ledcWrite(PWM_CHNL_THROTTLE, applied_throttle);
      prev_throttle = applied_throttle;
    }
    if (prev_steering != steering) {
      ledcWrite(PWM_CHNL_STEERING, steering);
    }
  }

  iteration++;
}
