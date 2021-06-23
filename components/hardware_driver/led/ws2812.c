#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <soc/rmt_struct.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <driver/gpio.h>
#include <stdio.h>
#include <driver/rmt.h>
#include <driver/adc.h>


/********************************
*               *
*     user's parameters     *
*               *
*********************************/
//1.RGB_LED: pin to control RGB_LED signal.
//2.LED_NUM: total RGB LEDs numbers.
#define RGB_LED 33
#define LED_NUM 12


/* static parameters */
//RMT_TICK_1_NS    (1/(80000000/RMT_CLK_DIV))   /*  RMT counter Source clock is APB clock */
//APB_CLK 80MHz, clock_cycle = 1/80/RMT_CLK_DIV = 25ns
//
//#define T0H   350ns/RMT_TICK_1_NS = 14
//#define T1H   700ns/RMT_TICK_1_NS = 32
//#define T0L   800ns/RMT_TICK_1_NS = 32
//#define T1L   800ns/RMT_TICK_1_NS = 14
//#define TRES  350us/RMT_TICK_1_NS = 14*1000


//#define RMT_CLK_DIV      2
//#define T0H   14
//#define T1H   28
//#define T0L   40
//#define T1L   32
//#define TRES    14*1000  //reset time


#define RMT_CLK_DIV      2
#define T0H     16
#define T1H     34
#define T0L     34
#define T1L     16
#define TRES    14*1000  //reset time 


#define BITS_PER_LED_CMD 24
#define LED_BUFFER_ITEMS ((LED_NUM * BITS_PER_LED_CMD))


#define LED_RMT_TX_CHANNEL RMT_CHANNEL_0


rmt_item32_t led_data_buffer[LED_BUFFER_ITEMS];


/*
1.define color union. rgb means one pixel which including three color: r/g/b.
2.refer csdn file for more details about "union".
3.little-endian.
*/
typedef union {

    struct __attribute__ ((packed)) {
        uint8_t r, g, b;
    };
    uint32_t rgb;
} rgb_color;



rgb_color set_LedRGB(uint8_t r, uint8_t g, uint8_t b)
{
    rgb_color v;

    v.r = r;
    v.g = g;
    v.b = b;
    return v;
}


//initial RMT structure parameters
void ws2812_control_init(gpio_num_t gpio_num)
{
    rmt_config_t config;
    config.rmt_mode = RMT_MODE_TX;
    config.channel = LED_RMT_TX_CHANNEL;
    config.gpio_num = gpio_num;
    config.mem_block_num = 3;
    config.tx_config.loop_en = false;
    config.tx_config.carrier_en = false;
    config.tx_config.idle_output_en = true;
    config.tx_config.idle_level = 0;
    config.clk_div = RMT_CLK_DIV;
    //ets_printf("~~~1\n");
    ESP_ERROR_CHECK(rmt_config(&config));
    ESP_ERROR_CHECK(rmt_driver_install(config.channel, 0, 0));
    //ets_printf("~~~2\n");
}


//
void setup_rmt_data_buffer(rgb_color *led_color)
{
    for (uint32_t led = 0; led < LED_NUM; led++)
    {
        uint32_t bits_to_send = led_color[led].rgb;
        uint32_t mask = 1 << (BITS_PER_LED_CMD - 1);
        for (uint32_t bit = 0; bit < BITS_PER_LED_CMD; bit++)
        {
            uint32_t bit_is_set = bits_to_send & mask;
            led_data_buffer[led * BITS_PER_LED_CMD + bit] = bit_is_set ?
(rmt_item32_t) {{{T1H, 1, T1L, 0}}} :
            (rmt_item32_t) {{{T0H, 1, T0L, 0}}};
            mask >>= 1;
        }
    }
}


//
void ws2812_write_leds(rgb_color *led_color) {
    setup_rmt_data_buffer(led_color);
        //ets_printf("~~~5\n");
    ESP_ERROR_CHECK(rmt_write_items(LED_RMT_TX_CHANNEL, led_data_buffer, LED_BUFFER_ITEMS, false));
        //ets_printf("~~~6\n");
    ESP_ERROR_CHECK(rmt_wait_tx_done(LED_RMT_TX_CHANNEL, portMAX_DELAY));
        //ets_printf("~~~7\n");
}


/*
1.blink all RGB leds.
*/
void breathing_light() {

    rgb_color *pixels;
    pixels = malloc(sizeof(rgb_color) * LED_NUM);

    while (1) {

        //blue blinks.
        printf("blue blinks...\n");
        for (int i = 0; i < LED_NUM; i++) {

            pixels[i] = set_LedRGB(100, 0, 0);

        }
        ws2812_write_leds(pixels);
        vTaskDelay(300 / portTICK_RATE_MS);

        for (int i = 0; i < LED_NUM; i++) {

            pixels[i] = set_LedRGB(0, 0, 0);

        }
        ws2812_write_leds(pixels);
        vTaskDelay(300 / portTICK_RATE_MS);


        //red blinks.
        printf("red blinks...\n");
        for (int i = 0; i < LED_NUM; i++) {

            pixels[i] = set_LedRGB(0, 100, 0);

        }
        ws2812_write_leds(pixels);
        vTaskDelay(300 / portTICK_RATE_MS);

        for (int i = 0; i < LED_NUM; i++) {

            pixels[i] = set_LedRGB(0, 0, 0);

        }
        ws2812_write_leds(pixels);
        vTaskDelay(300 / portTICK_RATE_MS);


        //green blinks.
        printf("green blinks...\n");
        for (int i = 0; i < LED_NUM; i++) {

            pixels[i] = set_LedRGB(0, 0, 100);

        }
        ws2812_write_leds(pixels);
        vTaskDelay(300 / portTICK_RATE_MS);

        for (int i = 0; i < LED_NUM; i++) {

            pixels[i] = set_LedRGB(0, 0, 0);

        }

        ws2812_write_leds(pixels);
        vTaskDelay(300 / portTICK_RATE_MS);

    }
}


void breathing_done() {

    rgb_color *pixels;
    pixels = malloc(sizeof(rgb_color) * LED_NUM);

    for (int i = 0; i < LED_NUM; i++) {

        pixels[i] = set_LedRGB(100, 100, 100);

    }
    ws2812_write_leds(pixels);
    vTaskDelay(300 / portTICK_RATE_MS);

}

void init_ws2812()
{
    // nvs_flash_init();
    ws2812_control_init(RGB_LED);
        //ets_printf("~~~3\n");
    rgb_color *pixels;
    pixels = malloc(sizeof(rgb_color) * LED_NUM);

    for (int i = 0; i < LED_NUM; i++)
    {
        pixels[i] = set_LedRGB(0, 0, 0);
    }
        //ets_printf("~~~4\n");
    ws2812_write_leds(pixels);
    //ets_printf("~~~8\n");
    free(pixels);
}

void RGB_1s()
{
    rgb_color *pixels;
    pixels = malloc(sizeof(rgb_color) * LED_NUM);

    // RED

    for (int i = 0; i < LED_NUM; i++) {

        pixels[i] = set_LedRGB(0, 100, 0);

    }
    ws2812_write_leds(pixels);
    vTaskDelay(1000 / portTICK_RATE_MS);

    for (int i = 0; i < LED_NUM; i++) {

        pixels[i] = set_LedRGB(0, 0, 0);

    }
    ws2812_write_leds(pixels);
    vTaskDelay(1000 / portTICK_RATE_MS);

    // GREEN

    for (int i = 0; i < LED_NUM; i++) {

        pixels[i] = set_LedRGB(0, 0, 100);

    }
    ws2812_write_leds(pixels);
    vTaskDelay(1000 / portTICK_RATE_MS);

    for (int i = 0; i < LED_NUM; i++) {

        pixels[i] = set_LedRGB(0, 0, 0);

    }
    ws2812_write_leds(pixels);
    vTaskDelay(1000 / portTICK_RATE_MS);

    // BLUE

    for (int i = 0; i < LED_NUM; i++) {

        pixels[i] = set_LedRGB(100, 0, 0);

    }
    ws2812_write_leds(pixels);
    vTaskDelay(1000 / portTICK_RATE_MS);

    for (int i = 0; i < LED_NUM; i++) {

        pixels[i] = set_LedRGB(0, 0, 0);

    }
    ws2812_write_leds(pixels);
}

bool waiting_for_command;
int light_command[3] = {0, 0, 0};

void wake_up_light_task(void *arg)
{
    rgb_color *pixels;
    pixels = malloc(sizeof(rgb_color) * LED_NUM);

    waiting_for_command = true;
    for (int i = 0; i < LED_NUM; i = (i + 1) % LED_NUM)
    {
        if (false == waiting_for_command)
            break;
        for (int j = 0; j < LED_NUM; j++)
        {
            if (j == i)
            {
                pixels[j] = set_LedRGB(100, 100, 100);
            }
            else
            {
                pixels[j] = set_LedRGB(0, 0, 0);
            }
        }
        ws2812_write_leds(pixels);
        vTaskDelay(200 / portTICK_RATE_MS);
    }

    free(pixels);
    vTaskDelete(NULL);
}

void wake_up_light()
{
    xTaskCreatePinnedToCore(&wake_up_light_task, "wul", 1024, NULL, 8, NULL, 1);
}

void return_light_state()
{
    rgb_color *pixels;
    pixels = malloc(sizeof(rgb_color) * LED_NUM);

    waiting_for_command = false;
    for (int i = 0; i < LED_NUM; i++)
    {
        pixels[i] = set_LedRGB(light_command[0], light_command[1], light_command[2]);
    }
    ws2812_write_leds(pixels);
    free(pixels);
}

void light_off()
{
    rgb_color *pixels;
    pixels = malloc(sizeof(rgb_color) * LED_NUM);

    waiting_for_command = false;
    light_command[0] = 0;
    light_command[1] = 0;
    light_command[2] = 0;
    for (int i = 0; i < LED_NUM; i++)
    {
        pixels[i] = set_LedRGB(light_command[0], light_command[1], light_command[2]);
    }
    ws2812_write_leds(pixels);
    free(pixels);
}

void white_light_on()
{
    rgb_color *pixels;
    pixels = malloc(sizeof(rgb_color) * LED_NUM);

    waiting_for_command = false;
    light_command[0] = 100;
    light_command[1] = 100;
    light_command[2] = 100;
    for (int i = 0; i < LED_NUM; i++)
    {
        pixels[i] = set_LedRGB(light_command[0], light_command[1], light_command[2]);
    }
    ws2812_write_leds(pixels);
    free(pixels);
}

void red_light_on()
{
    rgb_color *pixels;
    pixels = malloc(sizeof(rgb_color) * LED_NUM);

    waiting_for_command = false;
    light_command[0] = 0;
    light_command[1] = 100;
    light_command[2] = 0;
    for (int i = 0; i < LED_NUM; i++)
    {
        pixels[i] = set_LedRGB(light_command[0], light_command[1], light_command[2]);
    }
    ws2812_write_leds(pixels);
    free(pixels);
}

void green_light_on()
{
    rgb_color *pixels;
    pixels = malloc(sizeof(rgb_color) * LED_NUM);

    waiting_for_command = false;
    light_command[0] = 0;
    light_command[1] = 0;
    light_command[2] = 100;
    for (int i = 0; i < LED_NUM; i++)
    {
        pixels[i] = set_LedRGB(light_command[0], light_command[1], light_command[2]);
    }
    ws2812_write_leds(pixels);
    free(pixels);
}

void blue_light_on()
{
    rgb_color *pixels;
    pixels = malloc(sizeof(rgb_color) * LED_NUM);

    waiting_for_command = false;
    light_command[0] = 100;
    light_command[1] = 0;
    light_command[2] = 0;
    for (int i = 0; i < LED_NUM; i++)
    {
        pixels[i] = set_LedRGB(light_command[0], light_command[1], light_command[2]);
    }
    ws2812_write_leds(pixels);
    free(pixels);
}

void yellow_light_on()
{
    rgb_color *pixels;
    pixels = malloc(sizeof(rgb_color) * LED_NUM);

    waiting_for_command = false;
    light_command[0] = 0;
    light_command[1] = 100;
    light_command[2] = 100;
    for (int i = 0; i < LED_NUM; i++)
    {
        pixels[i] = set_LedRGB(light_command[0], light_command[1], light_command[2]);
    }
    ws2812_write_leds(pixels);
    free(pixels);
}

void orange_light_on()
{
    rgb_color *pixels;
    pixels = malloc(sizeof(rgb_color) * LED_NUM);

    waiting_for_command = false;
    light_command[0] = 0;
    light_command[1] = 100;
    light_command[2] = 25;
    for (int i = 0; i < LED_NUM; i++)
    {
        pixels[i] = set_LedRGB(light_command[0], light_command[1], light_command[2]);
    }
    ws2812_write_leds(pixels);
    free(pixels);
}

void purple_light_on()
{
    rgb_color *pixels;
    pixels = malloc(sizeof(rgb_color) * LED_NUM);

    waiting_for_command = false;
    light_command[0] = 50;
    light_command[1] = 50;
    light_command[2] = 0;
    for (int i = 0; i < LED_NUM; i++)
    {
        pixels[i] = set_LedRGB(light_command[0], light_command[1], light_command[2]);
    }
    ws2812_write_leds(pixels);
    free(pixels);
}