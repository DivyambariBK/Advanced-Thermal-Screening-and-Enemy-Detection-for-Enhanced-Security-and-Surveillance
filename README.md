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
```

### Why CSV?

- **Human-readable for debugging**  
  The data stream can be easily viewed and verified using a serial monitor during development and testing.

- **Robust against partial data corruption**  
  If a single frame is corrupted, it can be discarded without affecting subsequent frames.

- **Easy parsing on low-memory microcontrollers**  
  CSV strings can be parsed using simple string operations without complex buffer handling.

- **No endianness or binary alignment issues**  
  Avoids common problems associated with binary packet decoding across different architectures.

---

## 🎨 Thermal Color Mapping Logic

The system uses adaptive color mapping to clearly distinguish ambient temperature from human heat signatures:

- **Ambient heat** → Blue shades  
- **Moderate temperature delta (~1–2 °C)** → Green to yellow transition  
- **Human detection (>2 °C delta)** → Red shades  
- **Higher deltas** → Darker red (stronger and closer heat source)

This approach ensures **immediate visual recognition of human presence**, even at longer distances.

---

## 🧪 Applications

- Night patrol and perimeter security  
- Border surveillance and monitoring  
- Search and rescue operations  
- Firefighter visibility in smoke-filled environments  
- Industrial safety monitoring  
- Disaster response and victim detection  

> ⚠️ **Note:** The system is designed for *patrolling, surveillance, and detection* rather than active combat engagement.

---

## 🚀 Future Enhancements

- Higher-resolution thermal sensors (32×24 or higher)  
- Improved silhouette and contour extraction algorithms  
- Wireless data transmission to control rooms or command centers  
- Custom PCB design for reduced size and weight  
- AI-based human classification and tracking  
- Extended battery life and ruggedized enclosure  

---

## 📸 Project Demonstration

![WhatsApp Image 2025-12-08 at 10 17 35](https://github.com/user-attachments/assets/eb02f416-f0e6-47ab-b6a4-594ecf1f7d30)

![WhatsApp Image 2025-12-08 at 10 18 42](https://github.com/user-attachments/assets/7b042997-3ec1-4138-8acb-a89c1138bdaf)

