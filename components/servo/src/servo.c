#include "servo.h"
#include "driver/i2c_master.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static i2c_master_dev_handle_t pca_handle;

static const servo_cal_ticks ticks[SERVOS_NUMBER] = {
    {120, 510},
    {120, 510},
    {150, 500},
    {150, 500},
    {150, 500}
};


void i2c_init(){
    const i2c_master_bus_config_t i2c_master = {
        .scl_io_num = I2C_SCK_PIN,
        .sda_io_num = I2C_SDA_PIN,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port =  I2C_NUM_0,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true
    };
    i2c_master_bus_handle_t i2c_handle;
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_master, &i2c_handle));

    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = I2C_ADDRESS,
        .scl_speed_hz = 100000,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(i2c_handle, &dev_config, &pca_handle));
}

esp_err_t pca9685_write(uint8_t reg, uint8_t data){
    uint8_t buf[2] = {reg, data};
    return i2c_master_transmit(pca_handle, buf, sizeof(buf), 1000);
}


void init_pca9685(){
    pca9685_write(0x00, 0x10); // set sleep mode for pin to set prescaler
    vTaskDelay(pdMS_TO_TICKS(10));
    pca9685_write(0xFE, 121); //Set PWM Frequency to 50Hz
    pca9685_write(0x00, 0xA0); // Set auto increment registers and wake up
    vTaskDelay(pdMS_TO_TICKS(50));
}

void set_servo_angle(uint8_t channel, float angle){
    if (channel >= SERVOS_NUMBER) return;
    if (angle > 180.0f) angle=180.0f;
    if (angle < 0.0f) angle = 0.0f;

    uint16_t max_ticks = ticks[channel].max_ticks;
    uint16_t min_ticks = ticks[channel].min_ticks;
    uint16_t pulse_ticks = min_ticks + (uint16_t)((angle/180.0f)*(max_ticks-min_ticks));
    uint8_t reg = 0x06+(4*channel);
    uint8_t data_buf[5] = {reg, 0,0, pulse_ticks & 0xFF, pulse_ticks>>8};
    i2c_master_transmit(pca_handle, data_buf, sizeof(data_buf), 1000);
}

void move_all_servos(float rot_angles[SERVOS_NUMBER]){
    for (uint8_t ch=0; ch < SERVOS_NUMBER; ch++){
        set_servo_angle(ch, rot_angles[ch]);
        if (ch == 4) set_servo_angle(ch+1, rot_angles[ch+1]);
        vTaskDelay(pdMS_TO_TICKS(30)); // adds delay to soften current spikes
    }
}