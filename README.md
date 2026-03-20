# 🤖 AVR Line Follower Robot

> Autonomous line follower robot programmed in AVR-C using two IR 
> sensors and dual control modes on Arduino Nano.

---

## 📋 Overview
A fully functional line follower robot using **AVR-C** and **two IR 
sensors** to detect a black line. Operates in two selectable control 
modes via a wire bridge on PD4.

---

## ⚙️ Control Modes
| Mode | Type | Behaviour |
|------|------|-----------|
| **Mode 1** | Bang-Bang | Discrete on/off motor decisions |
| **Mode 2** | P-Control | Smooth proportional speed correction |

---

## 🔧 Features
- Sensor calibration via Button SW2, stored in **EEPROM**
- Speed adjustment via **POT1** in Bang-Bang mode
- P-Control gain tunable via `KP` define
- Mode selection via wire bridge between **PD4 and GND**

---

## 🔩 Hardware
- Arduino Nano (ATmega328P)
- 2x IR sensors (A0, A1)
- 2x DC motors with PWM speed control (Timer1)
- Potentiometer on A7 for speed control
- Breadboard + motor driver

---

## 🛠️ Tools Used
- Language: **AVR-C** (no Arduino libraries)
- IDE: **AVR Studio / Microchip Studio**
- Board: **Arduino Nano**

---

## 🎥 Demo Video
[![Watch on YouTube](https://img.shields.io/badge/YouTube-Watch%20Demo-red?logo=youtube)](https://youtube.com/shorts/h5qn4ogxA70?si=QxlQ6IfFX73gQrZh)
