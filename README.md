# Project HERA: Offline AI Smart Home Voice Hub

A fully customized, neural-network-driven offline voice controller built on the **ESP32-S3** microcontroller. Project HERA bypasses cloud listening latency by utilizing localized wake-word detection and custom acoustic phonetics to directly manage home automation via MultiNet AI.

---

## 🧠 System Architecture & Engineering Highlights

* **100% Offline Keyword Detection:** Powered by Espressif's `WakeNet9` engine running locally on the ESP32-S3 Korvo-1 hardware pipeline. Listen operations require zero cloud computation.

* **Custom Acoustic Phonetics:** Overcame heavy acoustic overlap bugs (e.g., false triggering between similar-sounding rooms) by reverse-engineering native G2P mapping. Custom multi-syllable logic ensures 90%+ command accuracy across the room.

* **Dual-Layer Automation Triggering:**
  * **Local WLED Webhooks:** Lightning-fast ambient lighting control over local HTTP (`192.168.1.X`), executing triggers in bare milliseconds without XML chunked-encoding crashes.
  * **Cloud Webhooks (Voice Monkey Bridge):** Background HTTPS `GET` requests linked to AWS/Alexa cloud triggers smoothly control Tuya smart relays (Floodlights & Dining room) without blocking the FreeRTOS audio feed task.

* **Native RGB Visual Feedback:** Built-in WS2812 NeoPixel integration (mapped accurately to Korvo-1 `GPIO 48`) provides immediate visual states: **Red** (Standby/Sleep), **Green** (Active Listening), and **Blue Flash** (Successful Transmission).

---

## 🎙️ Command Vocabulary Matrix

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

## ⚙️ Build & Flashing Instructions

1. **Framework:** Ensure Espressif **ESP-IDF v5.5+** is fully active within your environment.
2. **Scrubbed Parameters:** Prior to building, populate the credential parameters inside `main.c` with your targeted values:
   * `WIFI_SSID` & `WIFI_PASS`
   * `VM_TOKEN` (Secure Voice Monkey API Token)
   * `WLED_IP` (Target WLED Instance IP)
  
3. **Compilation:** Execute a full workspace clean to compile the customized vocabulary models cleanly, followed by flash execution:
   ```bash
   idf.py fullclean
   idf.py build flash monitor

---
## This system framework utilizes the open-source en_speech_commands_recognition operational base layer provided within the Espressif ESP-Skainet core repository, highly overhauled and optimized for independent Multi-API environments.
