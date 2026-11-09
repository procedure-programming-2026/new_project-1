#include <stdio.h>
#include "driver/gpio.h"
#include "FreeRTOS.h"
#include "gpio.h"
#include "esp_err.h"
#include "servo.h"

#define I2C_SCK_PIN GPIO_NUM_33;
#define I2C_SDA_PIN GPIO_NUM_32;
#define I2C_ADDRESS 0x40;

#define INPUT_PIN_1  GPIO_NUM_26;
#define INPUT_PIN_2  GPIO_NUM_27;

void gpio_config(){
    gpio_config_t gpio_con;
    gpio_con.mode = SPEED_MODE;
    gpio_con.intr_type = INTER_MODE;
    gpio_con.pin_bit_mask = (1ULL <<INPUT_PIN_1) | (1ULL << INPUT_PIN_2) | (1ULL << I2C_SCK_PIN) | (1ULL << I2C_SDA_PIN);
    gpio_con.pull_up_en = 
}
void app_main(void)
{
    gpio_config();
    setup_pwm();
}
