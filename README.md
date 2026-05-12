# Project HERA: Offline AI Smart Home Voice Hub

<img width="2748" height="1536" alt="Project_Hera_Image " src="https://github.com/user-attachments/assets/017dd3ab-762b-4e37-8c89-fce1cc61189b" />

A fully customized, neural-network-driven offline voice controller built on the **ESP32-S3** microcontroller. Project HERA bypasses cloud listening latency by utilizing localized wake-word detection and custom acoustic phonetics to directly manage home automation via MultiNet AI.

---

## System Architecture & Engineering Highlights

* **100% Offline Keyword Detection:** Powered by Espressif's `WakeNet9` engine running locally on the ESP32-S3 Korvo-1 hardware pipeline. Listen operations require zero cloud computation.

* **Custom Acoustic Phonetics:** Overcame heavy acoustic overlap bugs (e.g., false triggering between similar-sounding rooms) by reverse-engineering native G2P mapping. Custom multi-syllable logic ensures 90%+ command accuracy across the room.
 
* **Dual-Layer Automation Triggering:**
  * **Local WLED Webhooks:** Lightning-fast ambient lighting control over local HTTP (`192.168.1.X`), executing triggers in bare milliseconds without XML chunked-encoding responses crashing the socket.
  * **Cloud Webhooks (Voice Monkey Bridge):** Background HTTPS `GET` requests linked to AWS/Alexa cloud triggers smoothly control Tuya smart relays (Floodlights & Dining room) without blocking the FreeRTOS audio feed task.
   
* **Native RGB Visual Feedback:** Built-in WS2812 NeoPixel integration provides immediate visual states: **Red** (Standby/Sleep), **Green** (Active Listening), and **Blue Flash** (Successful Transmission).

---

## Hardware Requirements & Pinout Guide

### Required Components
* **Microcontroller:** ESP32-S3-Korvo-1 (v4.0) Audio Development Board
* **Microphone:** Integrated onboard single-mic pipeline
* **Status Indication:** Onboard WS2812 Addressable RGB LED
* **Power Supply:** 5V / 2A via USB-C (Dedicated power port recommended)
* **Target Hardware:** WLED Controller (Local) & Tuya Smart Switches (Cloud)

### Wiring & Pin Configuration Table
External peripherals and internal pipelines map strictly to the following ESP32-S3 GPIO allocations:

| Component / Interface | Hardware Pin Name | Assigned GPIO | Operational Notes |
| :--- | :--- | :--- | :--- |
| **I2S Audio Input (BCLK)** | `I2S_BCLK` | **GPIO 41** | Master clock interface for microphone array |
| **I2S Audio Input (WS/LRCK)** | `I2S_WS` | **GPIO 42** | Word Select / Channel framing |
| **I2S Audio Input (DATA_IN)** | `I2S_DIN` | **GPIO 40** | Raw PCM audio stream input |
| **Onboard RGB Status LED** | `WS2812_DATA` | **GPIO 48** | Native NeoPixel control pin for Korvo-1 v4 |
| **UART Console (TX/RX)** | `U0TXD` / `U0RXD` | **GPIO 43 / 44** | Hardware serial logging interface |

---

## Command Vocabulary Matrix

The MultiNet command engine has been strictly structured with punchy, zero-filler keywords to maximize processing confidence:

| Command ID | Action Target | Spoken English Command | Underlying Phonetic Translation | Trigger Route |
| :--- | :--- | :--- | :--- | :--- |
| **ID 0** | Front Area Pole | *"Turn on floodlight"* | `TkN nN FLoD LiT` | HTTPS (Voice Monkey) |
| **ID 1** | Front Area Pole | *"Turn off floodlight"* | `TkN eF FLoD LiT` | HTTPS (Voice Monkey) |
| **ID 2** | Hallway Relay | *"Turn on dining"* | `TkN nN DiNgN` | HTTPS (Voice Monkey) |
| **ID 3** | Hallway Relay | *"Turn off dining"* | `TkN eF DiNgN` | HTTPS (Voice Monkey) |
| **ID 4** | TV Backlight | *"Turn on ambient"* | `TkN nN aMBiYcNT` | Local HTTP (WLED) |
| **ID 5** | TV Backlight | *"Turn off ambient"* | `TkN eF aMBiYcNT` | Local HTTP (WLED) |
| **ID 6** | Master Execution | *"Turn on all lights"* | `TkN nN eL LiTS` | Dual Trigger (Cloud + Local) |
| **ID 7** | Master Execution | *"Turn off all lights"* | `TkN eF eL LiTS` | Dual Trigger (Cloud + Local) |

---

## Software Prerequisites & Setup Instructions

### 1. Development Environment
* Install **Visual Studio Code** with the **ESP-IDF Extension**.
* Ensure **ESP-IDF v5.5.4** (or highly compatible v5.x variants) is fully active and compiled within your Python environment.

### 2. Parameter Scrubbing & Configuration
Prior to compilation, navigate to `main/main.c` and populate the internal definitions with your local parameters:
```c
// Target Network Parameters
#define WIFI_SSID      "YOUR_WIFI_SSID"
#define WIFI_PASS      "YOUR_WIFI_PASSWORD"

// Cloud API & Local Instance Mapping
#define VM_TOKEN       "YOUR_VOICEMONKEY_TOKEN_HERE"
#define WLED_IP        "192.168.1.X" // Update with static local WLED IP
```
---

This system framework utilizes the open-source **en_speech_commands_recognition** operational base layer provided within the **Espressif ESP-Skainet** core repository, highly overhauled, re-mapped, and optimized for independent Multi-API environments.
