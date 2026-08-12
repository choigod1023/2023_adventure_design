# 2023 Adventure Design — A C-ITS Built with an RC Car

[한국어](README.md) · [日本語](README.ja.md) · **English**

Final project for **Adventure Design, Fall 2023**, School of Electrical & Electronic Engineering, Dongguk University.
It chains a Flask signal server, a WiFi traffic-light node, an nRF24L01 radio link, and a self-driving RC car to reproduce V2I (vehicle-to-infrastructure) cooperative driving from **C-ITS (Cooperative Intelligent Transport Systems)** on small-scale hardware.

## About

In a real C-ITS, road infrastructure (traffic lights) broadcasts its signal phase and remaining time over the air, and vehicles receive it and decide on their own whether to slow, stop, or go at the stop line. This project shrinks that flow down to four nodes.

```
[Flask signal server]  --HTTP/JSON-->  [light node (UNO R4 WiFi)]  --nRF24L01-->  [RF relay]  --Serial-->  [RC car]
   generates phase and                  LED, 7-seg, servo barrier                  relays the radio     stop-line IR + signal logic
   remaining time                       + radio broadcast                          signal                → autonomous stop/go
```

- The **Flask server** updates the signal phase (red/yellow/green) and countdown every 3 seconds and serves it as JSON
- The **traffic-light node** polls the server over WiFi, drives real LEDs, a 7-segment display, and a servo barrier, and broadcasts the current signal over nRF24L01
- The **RF relay** receives the radio signal and forwards it to the RC car over Serial
- The **RC car** combines its IR stop-line sensor with the received signal: it stops at the line on red/yellow and drives through on green (manual Bluetooth control is also possible)

## ✨ Features / behavior

| Node | File | Role |
|------|------|------|
| Signal server | `flask.py` | Phase/remaining-time state machine, JSON API |
| Traffic-light node | `traffic.ino` | Poll server → LED / 7-segment countdown / servo barrier + nRF24L01 broadcast |
| RF relay | `rf.ino` | Receive nRF24L01 → relay over Serial |
| RC car | `rccar.ino` | Bluetooth control + IR stop-line detection + signal-aware autonomous stop/go |

### Signal state machine (`flask.py`)
- `APScheduler` updates the signal at a **3-second interval** (`interval`, `Asia/Seoul`)
- Signal codes: **14 = red**, **15 = yellow**, **16 = green**
- On red, `time` counts 6→0 then switches to green; on green, 5→0 then yellow; yellow switches to red
- `GET /` returns `{ id, description, signal, time }` as JSON (`host=0.0.0.0`, `port=80`)

### Traffic-light node (`traffic.ino`)
- HTTP GET to the Flask server (`172.20.10.7:80`) over **WiFi (WiFiS3)**, parsing `signal` and `time` from the JSON response (`ArduinoJson`)
- **Three signal LEDs**: `r=pin 14`, `y=pin 15`, `g=pin 16`
- **7-segment (common anode)**: segment pins `{2,3,4,5,6,7,8,9}`, countdown rendered from the `digits[10][8]` patterns
- **Servo barrier** (`pin 10`): `write(100)` (lower) when entering red, `write(10)` (raise) when entering green
- **nRF24L01 transmit**: broadcasts the current signal character (`'r'`/`'y'`/`'g'`) toward the RC car

### RF relay (`rf.ino`)
- Receive-only nRF24L01 — passes the received string straight through to the RC car board with `Serial.write()`

### RC car control (`rccar.ino`)
- **Bluetooth (SoftwareSerial)** manual control: `'w'` = forward, `'x'` = stop
- **IR distance sensor** (`A1`) detects the stop line (analog value `< 400` → on the line)
- **Signal-aware autonomous driving** (`motor_drive()`):
  - Normal road (`adc_Ir > 400`) → drive per the Bluetooth command
  - Stop line detected + signal ≠ green (`'g'`) → **stop**
  - Signal = green → drive through

## 🛠 Tech stack / hardware

**Languages / frameworks**
- Python 3 — Flask, APScheduler (signal server)
- C++ (Arduino) — three `.ino` nodes

**Arduino libraries**
- `WiFiS3` (UNO R4 WiFi) · `ArduinoJson` · `Servo` — traffic-light node
- `SPI` / `nRF24L01` / `RF24` (nRF24L01 radio link) — traffic-light node and RF relay
- `SoftwareSerial` (Bluetooth) — RC car

**Hardware (as far as the code shows)**
- MCU: traffic-light node = Arduino **UNO R4 WiFi**; RC car / relay = Arduino (driving SPI and SoftwareSerial)
- nRF24L01 2.4GHz radio module (address `"00001"`, `RF24_PA_MIN`)
- 3 signal LEDs, 1 seven-segment display (common anode), 1 servo motor (barrier)
- 2 DC motors + motor driver, IR distance sensor, Bluetooth module

## 📌 Pinout (per the code)

**Traffic-light node (`traffic.ino`)**

| Pin | Connection | Notes |
|----|------|------|
| 14 / 15 / 16 | Signal LEDs red / yellow / green | `r` / `y` / `g` |
| 2, 3, 4, 5, 6, 7, 8, 9 | 7-segment segments | Remaining-time display |
| 10 | Servo motor | Barrier (lowered on red / raised on green) |
| 17 (CE) / 18 (CSN) | nRF24L01 | Signal transmit |

**RC car (`rccar.ino`)**

| Pin | Connection | Notes |
|----|------|------|
| A1 | IR distance sensor | Stop-line detection (threshold 400) |
| 10 / 11 | Motor A / B speed (PWM) | `M_SPEED=120`, balance `A_BAL=1`, `B_BAL=0.865` |
| 12 / 13 | Motor A / B direction | |
| 8 (RXD) / 7 (TXD) | Bluetooth (SoftwareSerial) | 9600bps, `'w'`/`'x'` commands |

**RF relay (`rf.ino`)**

| Pin | Connection |
|----|------|
| 7 (CE) / 8 (CSN) | nRF24L01 receive |

## 🚀 Usage

### 1. Run the Flask signal server
```bash
pip install flask apscheduler
python flask.py    # serves the signal API on 0.0.0.0:80
```

### 2. Flash the traffic-light node (`traffic.ino`)
- In the Arduino IDE, install the `WiFiS3`, `ArduinoJson`, `Servo`, and `RF24` libraries
- Change the WiFi SSID / PASSWORD and server IP (`server[]`) at the top of the file to your actual Flask server address
  (WiFi credentials can be kept separately in `arduino_secrets.h`)
- Upload to the UNO R4 WiFi

### 3. Flash the RF relay (`rf.ino`) and RC car (`rccar.ino`)
- Upload each to its own Arduino board
- Wire the relay's Serial output to the RC car board's Serial input
- After Bluetooth pairing, drive the car with `'w'`/`'x'`; it stops and starts autonomously at the stop line according to the traffic-light node's broadcast

## 📁 Structure

```
2023_adventure_design/
├─ flask.py       # Python Flask signal server (phase/remaining-time state machine + JSON API)
├─ traffic.ino    # traffic-light node (UNO R4 WiFi): poll server → LED/7-seg/servo + nRF24L01 broadcast
├─ rf.ino         # RF relay (nRF24L01 → Serial)
└─ rccar.ino      # RC car control (Bluetooth + IR stop line + signal-aware autonomous driving)
```

---

> This is a learning/educational project. WiFi SSID and password, server IP, and similar values are hardcoded for the demo setup (a personal hotspot), so be sure to adapt them to your own environment before using it.

---

## 👤 Contribution & development environment

| Item | Detail |
|---|---|
| **Contribution share** | **100%** (solo development) |
| **Commits** | 3 / 3 (mine / all human commits) |
| **Contributors** | 1 |

<sub>Contribution share is counted by commit author email; bot and automation commits are excluded.</sub>
