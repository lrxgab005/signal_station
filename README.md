# Signal Station

![Signal Station Image](signal_station_image.png)

## Overview

This project combines Arduino hardware with Python software to create an interactive audio visual experience with the signal station. 

## System Design

Multiple devices communicate via various protocols to create an interactive media system.

### Architecture

- **Arduino (User Interface)**  
  - Sends/receives serial command messages.

- **Raspberry Pi (Central Hub)**  
  - Sends/Receives serial messages from the Arduino  
  - Sends JSON-RPC commands to an Android device running Kodi  
  - Hosts a dedicated WiFi network  
  - Manages Bluetooth connectivity to the speaker box
  - Maintains audio file library

- **Android Device (Kodi Media Player)**  
  - Receives JSON-RPC commands from the Raspberry Pi to control video playback.
  - Maintains video file library

- **Bluetooth Speaker Box**  
  - Receives audio output from the Raspberry Pi via Bluetooth.

### Communication Flow

```text
      +-----------------------+
      |      Arduino UI       |
      +-----------+-----------+
                  ^ Serial CTRL
                  |   (USB)
                  v
    +----------------------------+
    |   Raspberry Pi             |
    |  - WiFi Access Point       |
    +-----+------------------+---+
          | JSON-RPC CTRL    | Audio Signal
          |     (WIFI)       | (Bluetooth)
          v                  v
+----------------+   +--------------------+
| Android Device |   |  Bluetooth Speaker |
|   (Kodi)       |   |          Box       |
+----------------+   +--------------------+
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

## Install Raspbian

Install Raspbian using [Raspberry Pi Imager](https://www.raspberrypi.com/software/). 

- Setup existing wifi network Auth on install

## Configure wifi (Raspbian)

### Set up serial console

Before trying to set the device in hotspot mode it's a good idea to enable USB
serial access to the console:

1. Add `dtoverlay=dwc2` under the `[all]` section in the file
   `/boot/firmware/config.txt`
2. Add `modules-load=dwc2,g_serial` at the end of the line (NOT on a new line!)
   in `/boot/firmware/cmdline.txt`
3. Enable terminal:
   `sudo systemctl enable serial-getty@ttyGS0.service` and then
   `sudo systemctl start serial-getty@ttyGS0.service`
4. After restart, the USB serial device should show up when plugged into a
   host (e.g. at `/dev/tty.usbmodem*` on OS X)
5. Connect via `minicom -c on -D /dev/tty.usbmodem101` and log in.

### Hotspot mode

NOTE: Interface must support `AP` mode (e.g. Raspberry Pi 4 does, but some USB
wifi connectors on Raspberry Pi 2 do not)

```bash
sudo iw list | grep "Supported interface modes" -A 8
```

Install packages and set up config files:

```bash
sudo apt install -y tmux hostapd dnsmasq

cat <<EOF | sudo tee /etc/hostapd/hostapd.conf
interface=wlan0
driver=nl80211
ssid=$(hostname)
hw_mode=g
channel=7
wmm_enabled=0
macaddr_acl=0
auth_algs=1
ignore_broadcast_ssid=0
wpa=2
wpa_passphrase=opensecret
wpa_key_mgmt=WPA-PSK
rsn_pairwise=CCMP
EOF

cat <<EOF | sudo tee /etc/dnsmasq.conf
interface=wlan0
dhcp-range=192.168.4.2,192.168.4.20,255.255.255.0,24h
domain=wlan
address=/gw.wlan/192.168.4.1
EOF
```

Create script to start/stop hotspot mode

```bash
cat <<'EOF' | sudo tee /usr/local/bin/wifi-mode.sh
#!/bin/bash
start_hotspot() {
    # Stop NetworkManager from managing wlan0
    nmcli radio wifi off
    sleep 1  # makes sure `rfkill list wifi` does show "Soft blocked: no"
    rfkill unblock wlan
    # Configure interface manually
    ip link set wlan0 down
    ip addr flush dev wlan0
    ip addr add 192.168.4.1/24 dev wlan0
    ip link set wlan0 up
    # seems we have to stop it first?
    systemctl stop hostapd
    # Start hotspot services
    systemctl start hostapd
    systemctl start dnsmasq
}
stop_hotspot() {
    # Stop hotspot services
    systemctl stop hostapd
    systemctl stop dnsmasq
    # Reset interface
    ip link set wlan0 down
    ip addr flush dev wlan0
    ip link set wlan0 up
    # Let NetworkManager take control again
    nmcli radio wifi on
}
case "$1" in
    start)
        start_hotspot
        ;;
    stop)
        stop_hotspot
        ;;
    *)
        echo "Usage: $0 {start|stop}"
        exit 1
esac
EOF
sudo chmod u+x /usr/local/bin/wifi-mode.sh

# test
sudo /usr/local/bin/wifi-mode.sh start
sudo /usr/local/bin/wifi-mode.sh stop

# debug
sudo hostapd -d /etc/hostapd/hostapd.conf
sudo systemctl status hostapd --no-pager -l
sudo journalctl -u hostapd --no-pager  # would complain if e.g. blocked
```

Finally set up service that enables hotspot at startup

```bash
cat <<'EOF' | sudo tee /etc/systemd/system/wifi-hotspot.service
[Unit]
Description=WiFi Hotspot Service
After=network.target NetworkManager.service

[Service]
Type=oneshot
RemainAfterExit=yes
ExecStart=/usr/local/bin/wifi-mode.sh start
ExecStop=/usr/local/bin/wifi-mode.sh stop

[Install]
WantedBy=multi-user.target
EOF

sudo systemctl status wifi-hotspot
sudo systemctl start wifi-hotspot
sudo systemctl stop wifi-hotspot
sudo systemctl enable wifi-hotspot
sudo systemctl disable wifi-hotspot
```

### Connect Bluetooth Speaker Box to Raspberry Pi

1. Ensure Bluetooth services are running 

```bash
sudo systemctl start bluetooth
sudo systemctl enable bluetooth
```

2. Start bluetoothctl to pair and connect

```bash
bluetoothctl
```

Then in the interactive prompt:

```bash
power on
agent on
default-agent
scan on
```

Wait until you see your speaker’s MAC address, e.g., XX:XX:XX:XX:XX:XX, then:

```bash
scan off
pair XX:XX:XX:XX:XX:XX
trust XX:XX:XX:XX:XX:XX
connect XX:XX:XX:XX:XX:XX
exit
```

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

