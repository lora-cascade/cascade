# Cascade Firmware

This repository contains the code for the Cascade firmware. This firmware is for the LILYGO LoRa32 boards that we are using for our project.

## Requirements

In order to build the Firmware, you must:
- Have Docker installed.

## Build and Run

First, make sure you have cloned the submodules for this repository.

```bash
git submodule update --init --recursive
```

To build the firmware, run the following command in the project directory.

```bash
docker compose up cascade_builder
```

To flash the firmware, run the following command in the project directory.

```bash
docker compose up cascade_flasher
```

Congrats! The power of Project Cascade's firmware is now in your hands.

## NOTE: Windows

You must install the USBIPD library on Windows in order to pass the serial port to the Docker container. Install it [here](https://learn.microsoft.com/en-us/windows/wsl/connect-usb).

After installing make sure Docker is not running and run the following commands in an elevated command prompt.

```bash
usbipd list
```

Note the BUSID of the connected serial port. The board should have a VID of `1a86` and a PID of `55d4`.

```bash
uspid attach --wsl --busid <BUSID>
```

Then start Docker once again and flashing should work correctly.