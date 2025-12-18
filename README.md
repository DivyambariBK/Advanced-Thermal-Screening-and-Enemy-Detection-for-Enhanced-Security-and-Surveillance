# 🔥 Advanced Thermal Screening and Enemy Detection for Enhanced Security and Surveillance

A wearable, multi-sensor night-vision and human-detection system designed to enhance situational awareness during night patrols, low-visibility surveillance, and search-and-rescue operations.

---

## 📌 Overview

The **Advanced Thermal Screening and Enemy Detection System** is a smart helmet-mounted surveillance solution that detects human presence in complete darkness, smoke, fog, and low-visibility environments. By fusing thermal imaging, distance measurement, and motion sensing, the system provides real-time thermal maps, directional alerts, and distance information to the user.

Unlike conventional night-vision systems that rely on visible or infrared light, this system works entirely on **thermal energy**, enabling reliable human detection even in visually obstructed environments.

---

## 🎯 Key Objectives

- Detect human presence in zero-visibility conditions  
- Visualize heat signatures as smooth thermal maps  
- Provide real-time distance estimation to detected targets  
- Offer intuitive directional guidance (Turn Left / Turn Right)  
- Demonstrate intelligent sensor fusion using embedded systems  

---

## ⚙️ System Architecture

The system is divided into two major modules:

### 1️⃣ Sensor Hub (ESP32)
Responsible for:
- Thermal data acquisition
- Motion and distance sensing
- Sensor fusion and preprocessing
- Data packet generation and transmission

### 2️⃣ Display Hub (Arduino UNO)
Responsible for:
- Receiving processed sensor data
- Thermal interpolation and visualization
- Human-shape heat mapping
- User-friendly display output

---

## 🧠 Technologies Used

- **Microcontrollers**
  - ESP32-WROOM
  - Arduino UNO

- **Sensors**
  - AMG8833 Thermal Sensor (8×8 IR Array)
  - TF-Mini LiDAR (Distance Measurement)
  - HC-SR501 PIR Sensors (Directional Motion Detection)

- **Display**
  - 3.5-inch TFT LCD (ILI9486 Driver)

- **Power**
  - 7.4 V Li-ion Battery
  - Buck Converter (Regulated 5 V Supply)

---

## 🔬 Working Principle

1. The thermal sensor captures an 8×8 heat grid of the environment.
2. Ambient temperature is dynamically learned as a baseline.
3. Temperature deltas are computed to isolate human heat signatures.
4. LiDAR provides precise distance to detected targets.
5. PIR sensors detect left/right motion for directional alerts.
6. ESP32 sends processed data to Arduino UNO via UART (CSV protocol).
7. Arduino UNO interpolates the thermal data and displays a smooth heatmap.
8. Human presence is highlighted using adaptive color thresholds.

---

## 🌡️ Thermal Interpolation Technique

To overcome the low resolution of the thermal sensor, **bilinear interpolation** is used on the Arduino UNO to upscale the 8×8 grid into a smoother 24×24 visualization. This significantly improves human-shape perception and reduces blockiness in the thermal display.

---

## 🔁 Communication Protocol

- **UART Serial Communication (115200 baud)**
- Request–Response Model:
  - Arduino UNO sends `'R'` to request a frame
  - ESP32 responds with one complete data frame

### Data Format (CSV)
```text
CSV:<64 thermal values>;DIST:<distance>;L:<left PIR>;R:<right PIR>
