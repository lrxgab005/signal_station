# Signal Station

## Overview

This project combines Arduino hardware with Python software to create a signal station. Follow the setup instructions below for both components.

## System Design

Multiple devices communicate via various protocols to create an interactive media system.

### Architecture

- **Arduino (User Interface)**  
  Sends/receives serial command messages.

- **Raspberry Pi (Central Hub)**  
  - Receives serial messages from the Arduino  
  - Forwards JSON-RPC commands to an Android device running Kodi  
  - Hosts a dedicated WiFi network  
  - Manages Bluetooth connectivity to the speaker box  

- **Android Device (Kodi Media Player)**  
  Receives JSON-RPC commands from the Raspberry Pi to control video playback.

- **Bluetooth Speaker Box**  
  Receives audio output from the Raspberry Pi via Bluetooth.

### Communication Flow

```text
+-----------------------+
|      Arduino UI       |
+-----------+-----------+
            ^ Serial CTRL
            | (USB)
            v
+----------------------------+
|   Raspberry Pi             |
|  - WiFi Access Point       |
+-----+------------------+---+
      | JSON-RPC CTRL    | Audio
      |  (WIFI)          | (Bluetooth)
      v                  v
+----------------+   +-----------------------+
| Android Device |   | Bluetooth Speaker Box |
|   (Kodi)       |   +-----------------------+
+----------------+
```

## Setup Arduino

## Arduino

### Setup IDE

1. Open Arduino IDE
2. Tools->Board->Arduino Uno
3. Tools->Port->(select the port where Arduino is connected)

### Upload code

1. Open the `.ino` file
2. Click "Upload" button (arrow icon)
3. Wait for "Upload complete" message

## Python

### Setup

Create virtual env

```bash
virtualenv env -p /opt/homebrew/bin/python3
```

Install requirements

```bash
source env/bin/activate
pip install -r requirements.txt
```

### Enable Git LFS

```sh
git lfs install
git lfs pull
```