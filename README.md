# Cartridge Player ESP32-S3 Client

The hardware companion for the Cartridge Player system. This runs on an ESP32-S3 to provide a physical interface and display for your media, connecting directly to the [Desktop Client](https://github.com/EstebanMagallonPerez/cartridgePlayerDesktopClient).

## Hardware
* **ESP32-S3 Microcontroller**
* **CrowPanel ESP32 E-paper 5.79-inch** (Display)
* **Physical Controls** (Buttons and knobs/rotary encoders)

## Features & Code Highlights
* **Status Display:** Receives and displays real-time "now playing" information and media stats from the desktop application.
* **Hardware Controls:** Reads physical inputs to control desktop playback.
* **LVGL Implementation:** Serves as a working reference for using LVGL on the 5.79-inch CrowPanel (which lacks official example code).
* **Optimized EPD Driver:** The default E-paper driver boilerplate has been streamlined for faster performance.
* **Partial Refresh Manager:** Includes a custom partial refresh manager optimized for standard usage. 

## Installation
1. Clone the repository: 
   ```bash
   git clone [https://github.com/EstebanMagallonPerez/cartridgePlayerESP32S3Client.git](https://github.com/EstebanMagallonPerez/cartridgePlayerESP32S3Client.git)
2. Open the project in your preferred IDE (Arduino IDE or PlatformIO).

3. Connect the ESP32-S3 via USB.

4. Compile and upload the firmware to the board.

## Usage
1. Keep the ESP32-S3 connected to your PC.

2. Launch the Desktop Client application.

3. The E-paper screen will automatically update with stats and "now playing" data as media loads on the PC.

4. Use the physical knobs and buttons on the device to control the desktop application.
