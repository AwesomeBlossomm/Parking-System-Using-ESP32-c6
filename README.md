
# Parking System Using ESP32-C6

A compact and clever parking sensor system using the **ESP32-C6**, 3 input sensors, and 3 outputs. Detects vehicle presence and responds accordingly—no frills, just function.

---

##  Hardware Overview

- **Controller**: ESP32-C6 microcontroller (Wi-Fi/BLE enabled)
- **Inputs**: 3 sensors (e.g., IR or ultrasonic) to detect vehicles
- **Outputs**: 3 actuators (e.g., LEDs, buzzer, servo) for signaling or barrier control

---

##  Features

| Feature              | Description                                                |
|----------------------|------------------------------------------------------------|
| Vehicle Detection    | Reads 3 sensor inputs to determine presence of a vehicle  |
| Responsive Outputs   | Activates corresponding output(s) -- maybe LEDs or servo  |
| Network-ready (optional) | ESP32-C6 can send status over Wi-Fi or BLE              |

---

##  Setup & Wiring

1. Connect the 3 sensors to ESP32-C6 GPIO pins (e.g., 15, 18, 19).
2. Connect outputs (LEDs, buzzer, servo) to output pins (e.g., 21, 22, 23).
3. Use common ground and 5 V / 3.3 V as appropriate.


---

## Installation

1. Install the [Arduino IDE](https://www.arduino.cc/en/software).
2. Add ESP32 board support by following the [Espressif documentation](https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html).
3. Open the `.ino` file in your IDE.
4. Select **ESP32-C6** as the board.
5. Upload and monitor via Serial.

---

## Developers: 
Justine Juliana G. Balla
Rean Joy S. Cicat
Mikylla B. Fabro
