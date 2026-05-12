/*
   Custom Single-Mic ESP-SR with Voice Monkey, Local WLED & RGB LED Feedback
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s_std.h" 

#include "esp_wn_iface.h"
#include "esp_wn_models.h"
#include "esp_afe_sr_iface.h"
#include "esp_afe_sr_models.h"
#include "esp_mn_iface.h"
#include "esp_mn_models.h"
#include "esp_board_init.h"
#include "speech_commands_action.h"
#include "model_path.h"
#include "esp_process_sdkconfig.h"

// --- LED INCLUDES ---
#include "led_strip.h"

// --- WIFI & HTTP INCLUDES ---
#include "esp_wifi.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "esp_http_client.h"
#include "esp_tls.h" 
#include "esp_crt_bundle.h" 

// --- NETWORK CREDENTIALS ---
#define WIFI_SSID      "YOUR_WIFI_SSID"
#define WIFI_PASS      "YOUR_WIFI_PASSWORD"

// --- API CREDENTIALS ---
#define VM_TOKEN       "YOUR_VOICEMONKEY_TOKEN_HERE"
#define WLED_IP        "YOUR_WLED_IP_ADDRESS - Ex: 192.168.1.X"

// --- LED SETTINGS ---
#define LED_STRIP_GPIO 48 
led_strip_handle_t led_strip;

int wakeup_flag = 0;
static const esp_afe_sr_iface_t *afe_handle = NULL;
static volatile int task_flag = 0;
srmodel_list_t *models = NULL;
i2s_chan_handle_t rx_handle; 

// =================================================================
// LED HELPER FUNCTIONS
// =================================================================
void set_led_color(int r, int g, int b) {
    led_strip_set_pixel(led_strip, 0, r, g, b);
    led_strip_refresh(led_strip);
}

void init_led() {
    led_strip_config_t strip_config = {
        .strip_gpio_num = LED_STRIP_GPIO,
        .max_leds = 1, 
    };
    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000, 
    };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    led_strip_clear(led_strip);
    set_led_color(50, 0, 0); // Start Dim Red
}

// =================================================================
// BACKGROUND TASK: LOCAL WLED (HTTP)
// =================================================================
esp_err_t _http_event_handler(esp_http_client_event_t *evt) {
    return ESP_OK; // Ignore WLED's chunked XML response
}

void wled_task(void *pvParameters) {
    int turn_on = (int)pvParameters; 
    
    char url[128];
    snprintf(url, sizeof(url), "http://%s/win&T=%d", WLED_IP, turn_on);

    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_GET, 
        .timeout_ms = 2000, 
        .event_handler = _http_event_handler,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);
    
    if (err == ESP_OK || err == ESP_ERR_HTTP_INCOMPLETE_DATA) {
        printf("\n---> Local WLED Triggered Instantly! <---\n");
        set_led_color(0, 0, 50); // Flash Blue for Success!
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    } else {
        printf("\n---> Failed to send WLED Signal: %s <---\n", esp_err_to_name(err));
    }
    
    set_led_color(50, 0, 0); // Return to Red
    esp_http_client_cleanup(client);
    vTaskDelete(NULL); 
}

void trigger_wled(int state) {
    xTaskCreatePinnedToCore(&wled_task, "wled_task", 4096, (void*)state, 5, NULL, 0);
}

// =================================================================
// BACKGROUND TASK: VOICE MONKEY (HTTPS)
// =================================================================
void voicemonkey_task(void *pvParameters) {
    char device_name[32];
    strncpy(device_name, (char*)pvParameters, sizeof(device_name) - 1);
    device_name[sizeof(device_name) - 1] = '\0';
    
    char url[256];
    snprintf(url, sizeof(url), "https://api-v2.voicemonkey.io/trigger?token=%s&device=%s", VM_TOKEN, device_name);

    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_GET, 
        .crt_bundle_attach = esp_crt_bundle_attach, 
        .timeout_ms = 5000, 
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);
    
    if (err == ESP_OK) {
        printf("\n---> Voice Monkey Triggered! Alexa is turning %s <---\n", device_name);
        set_led_color(0, 0, 50); // Flash Blue for Success!
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    } else {
        printf("\n---> Failed to send Voice Monkey Signal: %s <---\n", esp_err_to_name(err));
    }
    
    set_led_color(50, 0, 0); // Return to Red
    esp_http_client_cleanup(client);
    vTaskDelete(NULL); 
}

void trigger_voicemonkey(const char* device_name) {
    xTaskCreatePinnedToCore(&voicemonkey_task, "vm_task", 8192, (void*)device_name, 5, NULL, 0);
}

// =================================================================
// WIFI & AUDIO TASKS
// =================================================================
void wifi_init_sta(void) {
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_t *netif = esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();
    esp_wifi_connect();
    
    printf("\nConnecting to WiFi");
    esp_netif_ip_info_t ip_info;
    int retries = 0;
    
    while (1) {
        esp_netif_get_ip_info(netif, &ip_info);
        if (ip_info.ip.addr != 0) {
            printf("\n========================================\n");
            printf("   WiFi Connected Successfully! \n");
            printf("========================================\n\n");
            break;
        }
        printf(".");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        retries++;
        if(retries % 5 == 0) esp_wifi_connect(); 
    }
}

void feed_Task(void *arg) {
    esp_afe_sr_data_t *afe_data = arg;
    int audio_chunksize = afe_handle->get_feed_chunksize(afe_data);
    
    int16_t *i2s_buff = malloc(audio_chunksize * sizeof(int16_t));
    int32_t *raw_buff = malloc(audio_chunksize * sizeof(int32_t));
    
    while (task_flag) {
        size_t bytes_read = 0;
        if (i2s_channel_read(rx_handle, raw_buff, audio_chunksize * sizeof(int32_t), &bytes_read, portMAX_DELAY) == ESP_OK) {
            int samples_read = bytes_read / sizeof(int32_t);
            for (int i = 0; i < samples_read; i++) {
                i2s_buff[i] = raw_buff[i] >> 14; 
            }
            afe_handle->feed(afe_data, i2s_buff);
        }
    }
    free(i2s_buff);
    free(raw_buff);
    vTaskDelete(NULL);
}

void detect_Task(void *arg) {
    esp_afe_sr_data_t *afe_data = arg;
    int afe_chunksize = afe_handle->get_fetch_chunksize(afe_data);
    char *mn_name = esp_srmodel_filter(models, ESP_MN_PREFIX, ESP_MN_ENGLISH);
    esp_mn_iface_t *multinet = esp_mn_handle_from_name(mn_name);
    model_iface_data_t *model_data = multinet->create(mn_name, 6000);
    esp_mn_commands_update_from_sdkconfig(multinet, model_data); 

    printf("------------detect start------------\n");
    while (task_flag) {
        afe_fetch_result_t* res = afe_handle->fetch(afe_data); 
        if (!res || res->ret_value == ESP_FAIL) break;

        if (res->wakeup_state == WAKENET_DETECTED) {
            printf("WAKEWORD DETECTED\n");
            set_led_color(0, 50, 0); // Turn Green to show listening!
            multinet->clean(model_data);
        }

        if (res->raw_data_channels == 1 && res->wakeup_state == WAKENET_DETECTED) {
            wakeup_flag = 1;
        } else if (res->raw_data_channels > 1 && res->wakeup_state == WAKENET_CHANNEL_VERIFIED) {
            wakeup_flag = 1;
        }

        if (wakeup_flag == 1) {
            esp_mn_state_t mn_state = multinet->detect(model_data, res->data);

            if (mn_state == ESP_MN_STATE_DETECTING) continue;

            if (mn_state == ESP_MN_STATE_DETECTED) {
                esp_mn_results_t *mn_result = multinet->get_results(model_data);
                
                if (mn_result->num > 0) {
                    int best_guess_id = mn_result->command_id[0]; 
                    
                    // 1. Floodlight (Was Garden Light)
                    if (best_guess_id == 0) {
                        printf("\n*** ACTION: FLOODLIGHT ON ***\n");
                        trigger_voicemonkey("gardenlighton"); // Still triggers the same VoiceMonkey!
                    }
                    else if (best_guess_id == 1) {
                        printf("\n*** ACTION: FLOODLIGHT OFF ***\n");
                        trigger_voicemonkey("gardenlightoff"); 
                    }
                    
                    // 2. Dining Room Light
                    else if (best_guess_id == 2) {
                        printf("\n*** ACTION: DINING ON ***\n");
                        trigger_voicemonkey("dininglighton"); 
                    }
                    else if (best_guess_id == 3) {
                        printf("\n*** ACTION: DINING OFF ***\n");
                        trigger_voicemonkey("dininglightoff"); 
                    }
                    
                    // 3. WLED (Ambient)
                    else if (best_guess_id == 4) {
                        printf("\n*** ACTION: AMBIENT ON ***\n");
                        trigger_wled(1); 
                    }
                    else if (best_guess_id == 5) {
                        printf("\n*** ACTION: AMBIENT OFF ***\n");
                        trigger_wled(0); 
                    }
                    
                    // 4. Master Switch (All Lights)
                    else if (best_guess_id == 6) {
                        printf("\n*** ACTION: ALL LIGHTS ON ***\n");
                        trigger_voicemonkey("alllightson"); 
                        trigger_wled(1); 
                    }
                    else if (best_guess_id == 7) {
                        printf("\n*** ACTION: ALL LIGHTS OFF ***\n");
                        trigger_voicemonkey("alllightsoff"); 
                        trigger_wled(0); 
                    }
                }
                
                printf("-----------listening-----------\n");
            }

            if (mn_state == ESP_MN_STATE_TIMEOUT) {
                esp_mn_results_t *mn_result = multinet->get_results(model_data);
                set_led_color(50, 0, 0); // Return to Red
                afe_handle->enable_wakenet(afe_data);
                wakeup_flag = 0;
                printf("\n-----------awaits to be waken up-----------\n");
                continue;
            }
        }
    }
    if (model_data) multinet->destroy(model_data);
    vTaskDelete(NULL);
}

void app_main() {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    init_led(); 
    wifi_init_sta();
    models = esp_srmodel_init("model"); 

    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    i2s_new_channel(&chan_cfg, NULL, &rx_handle);

    i2s_std_config_t std_cfg = {
        .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(16000), 
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED, .bclk = 41, .ws = 42,
            .dout = I2S_GPIO_UNUSED, .din = 40,
            .invert_flags = {false, false, false},
        },
    };
    std_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_LEFT;
    i2s_channel_init_std_mode(rx_handle, &std_cfg);
    i2s_channel_enable(rx_handle);

    afe_config_t *afe_config = afe_config_init(esp_get_input_format(), models, AFE_TYPE_SR, AFE_MODE_LOW_COST);
    afe_config->pcm_config.total_ch_num = 1;
    afe_config->pcm_config.mic_num = 1;
    afe_config->pcm_config.ref_num = 0;
    afe_config->se_init = false;  
    afe_config->aec_init = false; 

    afe_handle = esp_afe_handle_from_config(afe_config);
    esp_afe_sr_data_t *afe_data = afe_handle->create_from_config(afe_config);
    afe_config_free(afe_config);

    task_flag = 1;
    xTaskCreatePinnedToCore(&detect_Task, "detect", 8 * 1024, (void*)afe_data, 5, NULL, 1);
    xTaskCreatePinnedToCore(&feed_Task, "feed", 8 * 1024, (void*)afe_data, 5, NULL, 0);
}