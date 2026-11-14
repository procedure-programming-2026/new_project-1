#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "servo.h"
#include "esp_log.h"


void app_main(void)
{
    const char*TAG ="FIVE SERVOS";
    i2c_init();
    init_pca9685();
    ESP_LOGI(TAG,"Starting channels 0-4!!!!");
    float pose_home[SERVOS_NUMBER] = {90.0f, 90.0f, 90.0f, 90.0f, 90.0f};
    float pose_reach[SERVOS_NUMBER] = {45.0f, 120.0f, 60.0f, 110.0f, 150.0f};
    while(1){
        move_all_servos(pose_home);
        vTaskDelay(pdMS_TO_TICKS(1500));
        move_all_servos(pose_reach);
        vTaskDelay(pdMS_TO_TICKS(1500));
        
    }
}
