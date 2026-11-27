#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "servo.h"
#include "esp_log.h"
#include "stdio.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "stdlib.h"
#include <string.h>

/* 1 = ТІльки логування отриманих повідомлень (без руху сервоприводів)
 0 = Повноцінне керування сервоприводів через PCA9685 */
#define TEST_MODE_ONLY_LOG 0

#define UART_PORT UART_NUM_0
#define UART_BAUD_RATE 115200
#define RX_BUF_SIZE 1024
#define STATUS_LED_GPIO GPIO_NUM_2

#define LINK_TIMEOUT_MS 500 //Час до фіксації втрати звєязку
#define SAFE_RETURN_DELAY_MS 3000 // Час до плавного повернення в Home
static const char*TAG = "FOTORUKA_ESP32";


static bool validate_and_parse(const char *line, char *dir_out, int *x_out, int *y_out, int *found_out, int *grip_out){
    if (line[0] != '$') return false;

    const char *star = strchr(line, '*');
    if (!star || strlen(star) < 3) return false;
    // Обчислюємо XOR між '$' та '*'
    unsigned char calc_cs = 0;
    for (const char *p = line+1; p<star; p++){
        calc_cs ^=(unsigned char)(*p);
    }
    unsigned int recv_cs = 0;
    if (sscanf(star+1, "%2x", &recv_cs)!= 1) return false;
    if (calc_cs != (unsigned char) recv_cs){
        ESP_LOGW(TAG, "Checksum mismatch! Calc: %02X, Recv: %02X", calc_cs, recv_cs);
        return false;
    }
    char payload[64];
    size_t payload_len = star-(line+1);
    if (payload_len >= sizeof(payload)) return false;
    memcpy(payload, line+1, payload_len);
    payload[payload_len] = '\0';
    
    if (sscanf(payload, "%15[^,],%d,%d,%d,%d", dir_out, x_out, y_out, found_out, grip_out) != 5){
        return false;
    }
    return true;
}

static float clamp_angle(float angle){
    if (angle < 10.0f ) return 10.0f;
    if (angle > 170.0f) return 170.0f;
    return angle;
}

void app_main(void)
{   
    //Налаштування світлодіода статусу підключення
    gpio_reset_pin(STATUS_LED_GPIO);
    gpio_set_direction(STATUS_LED_GPIO, GPIO_MODE_OUTPUT);

    //Налаштування UART
    uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
        .stop_bits = UART_STOP_BITS_1,
        .parity = UART_PARITY_DISABLE,
    };
    uart_driver_install(UART_PORT, RX_BUF_SIZE*2, 0,0, NULL, 0);
    uart_param_config(UART_PORT, &uart_config);
    uart_set_pin(UART_PORT, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_flush_input(UART_PORT);
    #if !TEST_MODE_ONLY_LOG
        esp_log_level_set("*", ESP_LOG_NONE);
        i2c_init();
        init_pca9685();
    #endif

    float pose_home[SERVOS_NUMBER] = {90.0, 90.0, 90.0, 90.0, 90.0};
    float current_angles[SERVOS_NUMBER] = {90.0, 90.0, 90.0, 90.0, 90.0};

    #if !TEST_MODE_ONLY_LOG
        move_all_servos(pose_home);
    #endif

    ESP_LOGI(TAG, "System started. Mode: %s", TEST_MODE_ONLY_LOG ? "TEST_ONLY" : "ACTIVE_SERVO");

    char line_buf[128];
    int line_pos = 0;
    uint8_t byte;
    TickType_t last_rx_tick = 0;
    bool is_connected = false;
    char current_dir[16] = "STOP";
    int x = -1, y = -1, found=0, grip=0;

    while(1){
        /*Читання байтів з таймаутом 20мс*/
        int len = uart_read_bytes(UART_PORT, &byte, 1, pdMS_TO_TICKS(20));
        if (len > 0){
            if (byte == '\n' || byte == '\r'){
                if (line_pos > 0){
                    line_buf[line_pos] = '\0';
                    //printf("ECHO: [%s]\n", line_buf);
                    line_pos = 0;
                    char parsed_dir[16];
                    int px, py, pfound, pgrip;
                    if (validate_and_parse(line_buf, parsed_dir, &px, &py, &pfound, &pgrip)){
                        last_rx_tick = xTaskGetTickCount();
                        if (!is_connected) {
                            is_connected = true;
                            gpio_set_level(STATUS_LED_GPIO, 1);
                            ESP_LOGI(TAG, "[STATUS] PC CONNECTED");
                        }

                        strcpy(current_dir, parsed_dir);
                        x =px, y=py, found=pfound, grip=pgrip;

                        #if !TEST_MODE_ONLY_LOG
                            ESP_LOGI(TAG, "[TEST RX] Dir=%s | X=%d | Y=%d | Found=%d", current_dir, x, y, found);
                        #endif
                    }else{
                        ESP_LOGW(TAG, "[ERROR] Malformed packet dropped: %s", line_buf);
                    }
                }
            } else {
                if (line_pos < (int)sizeof(line_buf)-1){
                    line_buf[line_pos++] = (char)byte;
                } else{
                    /* Захист від переповнення буфера*/
                    line_pos = 0;
                    uart_flush_input(UART_PORT);
                    ESP_LOGW(TAG, "[ERROR] Line buffer overflow, flushed");
                }
            }
        }
        TickType_t now = xTaskGetTickCount();
        uint32_t elapsed_ms = pdTICKS_TO_MS(now - last_rx_tick);

        if (is_connected && elapsed_ms > LINK_TIMEOUT_MS){
            is_connected = false;
            strcpy(current_dir, "STOP");
            found =0;
            gpio_set_level(STATUS_LED_GPIO, 0);
            ESP_LOGW(TAG, "[FAIL-SAFE] Connection lost (>%d ms)! Entering SAFE STATE (HOLD).", LINK_TIMEOUT_MS);
        }
        #if !TEST_MODE_ONLY_LOG
        if (is_connected ) {
            /* GRIPPER CONTROL (Channel 4): runs even if briefly leaves frame */
            float target_grip = (grip == 1) ? 125.0f :45.0f;
            if (current_angles[4] <target_grip){
                current_angles[4] += 4.0f;
                if (current_angles[4] > target_grip) current_angles[4] = target_grip;
            }else if (current_angles[4] > target_grip){
                    current_angles[4] -= 4.0f;
                    if (current_angles[4] < target_grip) current_angles[4] = target_grip;
                }
            if (found){
                float speed_base = 2.0f;
                float speed_arm = 1.5f;

                float dir_x = 1.0f;
                float dir_y = 1.0f;
                if(strcmp(current_dir, "LEFT") == 0) current_angles[0] = clamp_angle(current_angles[0]+dir_x*speed_base);
                if(strcmp(current_dir, "RIGHT") == 0) current_angles[0] = clamp_angle(current_angles[0]-dir_x*speed_base);
                if(strcmp(current_dir, "UP") == 0) {
                    current_angles[1] = clamp_angle(current_angles[1]+dir_y*speed_arm);
                    current_angles[2]=clamp_angle(current_angles[2] - dir_y*(speed_arm*0.8f));
                    current_angles[3]= clamp_angle(current_angles[3]+dir_y*(speed_arm * 0.5f));
                }
                if(strcmp(current_dir, "DOWN") == 0) {
                    current_angles[1] = clamp_angle(current_angles[1]-dir_y * (speed_arm));
                    current_angles[2] = clamp_angle(current_angles[2]+dir_y*(speed_arm*0.8f));
                    current_angles[3] = clamp_angle(current_angles[3]-dir_y*(speed_arm*0.5f));
                }

            }
            move_all_servos(current_angles);
        }else if (!is_connected && elapsed_ms >SAFE_RETURN_DELAY_MS){
                /* Плавне повернення у безпечну позицію Home після 3 секунд без зв'язку */
                bool moved = false;
                for (int i =0; i <SERVOS_NUMBER; i++){
                    if (current_angles[i] < pose_home[i]-1.0f){current_angles[i]+=1.0f; moved = true; }
                    else if (current_angles[i] > pose_home[i]+1.0f){current_angles[i]-=1.0f; moved = true; }
                }
                if (moved) move_all_servos(current_angles);
            }
        }
        #endif
    }
