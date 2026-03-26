# Heltec WiFi LoRa 32 V3 + NEO-7M GPS

A clean ESP-IDF project for **Heltec WiFi LoRa 32 V3 (ESP32-S3)** that reads GPS data from a NEO-7M module and displays it beautifully on the built-in OLED display.

![OLED Display](https://via.placeholder.com/600x300?text=GPS+on+OLED) <!-- You can replace this with a real screenshot later -->

## Features

- Reads real-time GPS data from NEO-6M / NEO-7M module
- Parses GGA NMEA sentences (Latitude, Longitude, Satellites, UTC Time, Fix status)
- Displays information clearly on the 128x64 OLED
- Proper power management (Vext control)
- Clean and well-commented code
- Built with ESP-IDF v5.5.2

## Hardware Requirements

- **Heltec WiFi LoRa 32 V3** (ESP32-S3)
- **NEO-7M** (or NEO-6M) GPS module
- Jumper wires

### Wiring

| NEO-7M Pin | Heltec V3 Pin     |
|------------|-------------------|
| VCC        | 3.3V or 5V        |
| GND        | GND               |
| TX         | GPIO45 (RX)       |
| RX         | GPIO46 (TX)       |

> **Note**: The OLED is internally connected (SDA=GPIO17, SCL=GPIO18, RST=GPIO21)

## Software Requirements

- ESP-IDF v5.5.2 or later
- VS Code + ESP-IDF Extension (recommended)

## How to Build & Flash

1. Clone the repository:
   ```bash
   git clone https://github.com/yourusername/Heltec_LoRa_WiFi_GPS.git
   cd Heltec_LoRa_WiFi_GPS
