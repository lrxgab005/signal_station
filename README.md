# Signal Station

## Overview

This project combines Arduino hardware with Python software to create a signal station. Follow the setup instructions below for both components.

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