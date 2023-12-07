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

Then, simply run the following command in the project directory.

```bash
docker compose up
```

Congrats! The power of project Cascade's firmware is now in your hands.