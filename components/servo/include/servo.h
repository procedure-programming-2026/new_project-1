
#ifndef SERVO_H
#define SERVO_H
#include "esp_err.h"
#include <stdint.h>
#include "driver/gpio.h"
#define I2C_SCK_PIN GPIO_NUM_33
#define I2C_SDA_PIN GPIO_NUM_32
#define I2C_ADDRESS 0x40
#define SERVOS_NUMBER 5
//SERVO CONSTANTS
#define MAX_ROTATION_ANGLE 90
esp_err_t pca9685_write(uint8_t reg, uint8_t data);
void i2c_init();
void init_pca9685();
void set_servo_angle(uint8_t channel, float angle);
typedef struct  {
    uint16_t min_ticks;
    uint16_t max_ticks;
} servo_cal_ticks;

void move_all_servos(float rot_angles[SERVOS_NUMBER]);

#endif