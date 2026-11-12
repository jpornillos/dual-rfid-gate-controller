# Dual-RFID Turnstile Access Control System

An Arduino-based access control controller that authenticates entry and exit RFID scans against a remote HTTP API in real time and actuates dual solenoid locks via a 2-channel relay module.

## Overview

The system uses two MFRC522 RFID readers connected over SPI to handle bidirectional gate traffic (`IN` and `OUT`). When a card or tag is scanned, the microcontroller normalizes the UID bytes into an 8-digit decimal identifier and sends an HTTP `GET` request via an Ethernet Shield. Based on the server's single-character response (`1` for authorized, `0` for denied), the controller triggers the corresponding solenoid relay and emits audible buzzer feedback.

## Hardware Components

* **Microcontroller:** 1x Arduino UNO
* **Networking:** 1x Arduino UNO Ethernet Shield (W5100/W5500)
* **RFID Readers:** 2x MFRC522 RFID Modules (13.56 MHz)
* **Actuation:** 1x 2-Channel 5V Relay Module
* **Locks:** 2x 12V Solenoid Door Locks (with external power supply)
* **Audio Feedback:** 1x Piezo/Active Buzzer

## Wiring & Pinout

| Component | Module Pin | Arduino UNO Pin | Notes |
| :--- | :--- | :--- | :--- |
| **RFID Reader 1 (`IN`)** | `SDA (SS)` | `D5` | Slave Select for Entry Reader |
| **RFID Reader 1 (`IN`)** | `RST` | `D9` | Reset pin |
| **RFID Reader 2 (`OUT`)** | `SDA (SS)` | `D4` | Slave Select for Exit Reader |
| **RFID Reader 2 (`OUT`)** | `RST` | `D6` | Reset pin |
| **Shared SPI Bus** | `MOSI / MISO / SCK` | `D11 / D12 / D13` | Shared between both MFRC522 readers and the Ethernet Shield (`D10` reserved for Ethernet `SS`) |
| **RFID Power** | `3.3V / GND` | `3.3V / GND` | MFRC522 operates at 3.3V logic/power |
| **Relay Module** | `IN1` (Entry Lock) | `A4` | Configured as digital output |
| **Relay Module** | `IN2` (Exit Lock) | `A5` | Configured as digital output |
| **Buzzer** | `+ (Signal)` | `D7` | Audible status indicator |

## Buzzer Status Signals

| Pattern | Meaning |
| :--- | :--- |
| **1 Beep** | System initialized and ready / Access granted (`1`) |
| **2 Beeps** | Access denied (`0`) |
| **3 Beeps** | Ethernet DHCP failure or server connection timeout |

## Setup & Configuration

### 1. Required Libraries
Install the following via the **Arduino IDE Library Manager**:
* `MFRC522` by GitHubCommunity / Miguel Balboa
* `SPI` *(built-in)*
* `Ethernet` *(built-in)*

### 2. Network & API Settings
Update the placeholder constants at the top of `turnstile.ino` before flashing:

```cpp
const char SERVER_HOST[] = "access-control.example.com";
const char API_ENDPOINT[] = "/api/check_entry";
const char GATE_LOCATION[] = "gate_01";
