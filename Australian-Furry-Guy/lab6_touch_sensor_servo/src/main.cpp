#include <Arduino.h>

#define ch_servo 0
#define freq_servo 50
#define pin_servo 37
#define MIN_DC 20
#define MAX_DC 130
#define DC_STEP 5

volatile int dc_servo = MIN_DC;
bool flag_increase = 1;

const char pin_inc = T8;
const char pin_dec = T3;

#define thr_touch 100e3

volatile bool flag_inc_old = 0;
volatile bool flag_dec_old = 0;

volatile float servo_angle = 90;

const char angle_step = 10;

volatile bool led_on = 0;

void setup()
{
  ledcSetup(ch_servo, freq_servo, 10);
  ledcAttachPin(pin_servo, ch_servo);

  dc_servo = map(servo_angle, 0, 180, MIN_DC, MAX_DC);
  ledcWrite(ch_servo, dc_servo);

  pinMode(2, OUTPUT);

  Serial.begin(9600);
}

void loop()
{
  if (touchRead(pin_inc) > thr_touch && !flag_inc_old)
  {
    delay(20);
    if (touchRead(pin_inc) > thr_touch && !flag_inc_old)
    {
      flag_inc_old = 1;
      digitalWrite(2, 1);
      servo_angle = constrain(servo_angle + angle_step, 0, 180);
      dc_servo = map(servo_angle, 0, 180, MIN_DC, MAX_DC);
      ledcWrite(ch_servo, dc_servo);
    }
  }

  if (touchRead(pin_dec) > thr_touch && !flag_dec_old)
  {
    delay(20);
    if (touchRead(pin_dec) > thr_touch && !flag_dec_old)
    {
      flag_dec_old = 1;
      digitalWrite(2, 1);
      servo_angle = constrain(servo_angle - angle_step, 0, 180);
      dc_servo = map(servo_angle, 0, 180, MIN_DC, MAX_DC);
      ledcWrite(ch_servo, dc_servo);
    }
  }

  if (touchRead(pin_inc) < thr_touch && flag_inc_old)
  {
    delay(20);
    if (touchRead(pin_inc) < thr_touch && flag_inc_old)
    {
      flag_inc_old = 0;
      digitalWrite(2, 0);
    }
  }

  if (touchRead(pin_dec) < thr_touch && flag_dec_old)
  {
    delay(20);
    if (touchRead(pin_dec) < thr_touch && flag_dec_old)
    {
      flag_dec_old = 0;
      digitalWrite(2, 0);
    }
  }

  Serial.printf("%d, %d\n", flag_inc_old, flag_dec_old);

  // delay(1000);
}
