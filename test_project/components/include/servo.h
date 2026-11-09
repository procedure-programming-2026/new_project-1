#include "driver/ledc.h"
#include <stdio.h>

#define LEDC_TIMER LEDC_TIMER_0
#define FREQUENCY 60
#define CHANNEL LEDC_CHANNEL_0
#define SPEED_MODE LEDC_LOW_SPEED_MODE
#define DUTY_RESOLUTION LEDC_TIMER_13_BIT
#define CLK_CONFIG LEDC_AUTO_CLK
#define INTER_MODE LEDC_INTR_DISABLE

//SERVO CONSTANTS
#define SERVO_MIN_PULSEWIDTH 1120 // MIN PULSEWIDTH to setup angle( 0 )
#define SERVO_MAX_PULSEWIDTH 1920 // MAX PULSEWIDTH to setup angle( 90 )
#define MAX_ROTATION_ANGLE 90

void setup_pwm(uint8_t pin_num);

void set_servo_angle(int angle);