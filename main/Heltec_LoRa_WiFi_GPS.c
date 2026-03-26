#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/uart.h"    // For GPS Communication
#include "esp_rom_sys.h"
#include "ssd1306.h"       // For OLED Display

// ------------------------------------------------------------
//  Project Specific Configuration Constants
// ------------------------------------------------------------

#define GPS_UART_PORT_NUM      UART_NUM_1     // Using UART1 (UART0 is usually for console)
#define GPS_TX_PIN             GPIO_NUM_46    // Heltec TX Pin -> connect to GPS RX
#define GPS_RX_PIN             GPIO_NUM_45    // Heltec RX Pin -> connect to GPS TX
#define GPS_BAUDRATE           9600           // Default Baudrate for NEO-6M / NEO-7M

#define VEXT_GPIO              GPIO_NUM_36    // Heltec VEXT power control pin (3.3V to OLED & sensors)
#define OLED_RESET_GPIO        GPIO_NUM_21    // Hardware reset pin of the builtin SSD1306 OLED



void app_main(void)
{

}
