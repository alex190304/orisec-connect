# OrisecConnect / Orisec2HA

**An open-source solution for integrating Orisec intruder alarm systems with Home Assistant, MQTT, and other automation platforms.**

---

## Overview

This project provides an open-source hardware and firmware solution for interfacing **Orisec intruder alarm panels** with modern home automation systems.

It consists of two main parts at the moment:

- **OrisecConnect** — an ESP32-based interface board designed to connect directly to Orisec control panels via their onboard UART com ports and proprietary CID Serial protocol
- **Orisec2HA** — open-source firmware that exposes zone statuses and allows control of areas and remote outputs in Home Assistant via MQTT

---

## Project Components

### OrisecConnect (Hardware)

Small, easy to use communications board for direct connection to the com port on Orisec intruder panels.

- ESP32-based interface board  
- Designed specifically for Orisec intruder alarm panels
- Open hardware design (schematics, PCB, BOM)  
- Intended to be sold as a complete board in the future for those not willing to build it
- Could easily be replicated with a dev-board for an easy DIY setup

Hardware design files are located in the `hardware/` directory.

---

### Orisec2HA (Firmware)

This firmware, created to run on the OrisecConnect board, uses Orisec's CID Serial protocol through an onboard UART com port to communicate with the alarm system.

- ESP32 firmware developed to run on the OrisecConnect board
- Local web interface via Wifi AP for setting variables
- Publishes data via MQTT
- Designed for seamless integration with Home Assistant  
- No cloud dependency
- Local-first and fully user-controlled

This firmware allows the following
- Realtime zone status with zone text
- Realtime area status with arm/part arm/disarm/reset commands
- Realtime remote output status and control
- Reading system voltages

Firmware source code is located in the `firmware/` directory.

> [!NOTE]
> The reason for structuring the hardware and software separately under the umbrella of Orisec Connect is to allow room for further integrations in the future instead of just limiting this board to being used for Home Assistant integrations.
---

## Features

- Local integration (no external cloud services)  
- MQTT-based communication  
- Home Assistant friendly (full discovery)
- Open-source hardware and software

Feature availability depends on the specific Orisec panel and configuration.

---

## Project Status

Work in progress.

This project is under active development.  
Hardware revisions, firmware features, and documentation may change.

Early testing, feedback, and contributions are welcome.

---

## Repository Structure

├── hardware/     # OrisecConnect PCB, schematics, BOM  
├── firmware/     # Orisec2HA ESP32 firmware  
├── docs/         # Setup guides and documentation  
└── LICENSE.md    # Licensing overview  

---

## Getting Started

High-level steps:

1. Either build an **OrisecConnect** board or setup a dev-board mimicing the connections
2. Flash the **Orisec2HA** firmware onto the ESP32
3. Activate config mode and connect to the Wifi AP
4. Use the built in web interface to configure MQTT and Home Assistant  
6. Integrate alarm entities into your automations  

Detailed instructions, panel compability and more are available in the `docs/` directory.

---

## Licensing

This repository contains multiple components under different licenses:

- **Hardware (`hardware/`)**  
  CERN Open Hardware Licence v2 – Weakly Reciprocal (CERN-OHL-W v2)

- **Firmware (`firmware/`)**  
  GNU General Public License v3.0 (GPL-3.0)

- **Documentation (`docs/`)**  
  Creative Commons Attribution-ShareAlike 4.0 (CC BY-SA 4.0)

---

## Disclaimer

Orisec is a registered trademark of Orisec Ltd. This project is not affiliated with Orisec at all, although they are aware of the existence and have provided their blessing for it to be made public.

This project is not affiliated with, endorsed by, or supported by Orisec Ltd.  
It is an independent, community-driven open-source project.
