![Language: C++](https://img.shields.io/badge/Language-C%2B%2B-blue?logo=c%2B%2B&logoColor=white) ![IDE: Arduino](https://img.shields.io/badge/IDE-Arduino-00979D?logo=arduino&logoColor=white) ![Platform: ESP32-S2](https://img.shields.io/badge/Platform-ESP32--S2-df2a2a?logo=espressif&logoColor=white)

🐉 EJDER V2.1 - Ultimate ESP32 Pentest Tool
EJDER V2.1 is a high-performance wireless security testing framework developed for the ESP32 microcontroller. It focuses on auditing Wi-Fi network resilience through advanced 802.11 frame injection, including deauthentication, beacon flooding, and probe request management.

⚠️ LEGAL DISCLAIMER
This software is provided for EDUCATIONAL AND RESEARCH PURPOSES ONLY. Unauthorized use of this tool against networks without explicit permission is strictly prohibited and may be illegal. The developer (SirAtilotty) holds no responsibility for any misuse, legal consequences, or hardware damage resulting from this software.

🚀 Technical Features
Lethal Deauth Burst: Implements aggressive deauthentication sequences using multiple reason codes (1, 4, 6, 7, 8) to bypass client-side reconnection logic.

Null Frame Injection: Disrupts target station processing by saturating the airtime with raw null data frames.

High-Density Beacon Flood: Generates dozens of spoofed Access Points (APs) simultaneously across multiple channels.

Probe Request Saturation: Floods the environment with fake probe requests to analyze network monitoring response.

Asynchronous Web Interface: Fully responsive control dashboard hosted at 192.168.4.1 for real-time attack management.

Optimized Timing: Microsecond-level delays for maximum packet throughput and efficient resource utilization.

🛠️ Hardware & Deployment
Hardware Requirements
Platform: Any standard ESP32 development board.

Radio: 2.4 GHz Wi-Fi (802.11 b/g/n).

Installation Procedure
Configure the ESP32 board support in your development environment (Arduino IDE or PlatformIO).

Open the EJDERV2_1.ino project file.

Include required internal libraries: WiFi, WebServer, esp_wifi.

Flash the firmware to the target ESP32 device.

Establish a connection to the SSID: EJDER_ULTIMATE (Default Password: EJDERYA32).

Access the dashboard via browser at: 192.168.4.1.

⚖️ License & Intellectual Property
This project is licensed under the GNU General Public License v3.0 (GPL-3.0).

Copyright (c) 2026 Atilla Ilhan (SirAtilotty)

Notice: Any redistribution, modification, or forking of this source code MUST retain the original copyright notice and credit to SirAtilotty. The license file must remain intact in all derivative works.

👨‍💻 Developer Information
Primary Developer: SirAtilotty

Project Status: Active Development
