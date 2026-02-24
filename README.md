# 🛡️ WBAN: AI-Powered Smart Safety System

![Architecture Diagram](Nesso_N1/arch.png)

## 📖 Overview

The **WBAN (Wireless Body Area Network)** project is a dual-node wearable safety system designed to detect distress signals and falls automatically. It consists of a **Smart Watch (Nesso N1)** and a **Smart Dress (Velostat Sensor Node)** that communicate wirelessly to provide real-time protection.

The system uses **Edge AI** to detect falls and "Panic Squeezes" on the dress, triggering a hybrid alert system that sends **SMS (GSM)** and **Web Alerts (WiFi)** with live **GPS location**.

---

## 🚀 Key Features

* **🤖 Edge AI Fall Detection:** Runs a custom TFLite model on the Nesso N1 (ESP32-C6) to detect falls using 6-axis IMU data.
* **👗 Smart Dress Integration:** A Velostat-based pressure sensor embedded in clothing sends data to the watch via **ESP-NOW** (Low Latency).
* **📡 Hybrid Connectivity:**
* **Primary:** Sends JSON alerts to a NestJS Backend via WiFi.
* **Failover:** Automatically switches to **GSM (SMS)** if WiFi is unavailable.


* **📍 Real-Time Tracking:** Captures GPS coordinates and generates a clickable Google Maps link.
* **🆘 SOS Panic Button:** Physical button on the watch for immediate manual activation.
* **🖥️ Live Dashboard:** React-based web interface for monitoring device status and alerts.

---

## 🛠️ Hardware Architecture

### 1. The Watch (Central Node)

* **Core:** Arduino Nesso N1 (ESP32-C6 SoC).
* **Sensors:** Built-in BMI270 (IMU) for fall detection.
* **Connectivity:**
* **GSM Module:** SIM800L (UART) for SMS.
* **GPS Module:** NEO-6M (UART) for Location.


* **Power:** 800mAh LiPo Battery (Y-Splice powering both GSM & Nesso).

### 2. The Dress (Sensor Node)

* **Core:** Seed Studio XIAO ESP32 / ESP32-C3.
* **Sensor:** Velostat Pressure Sensor (Conductive fabric).
* **Communication:** ESP-NOW (Peer-to-Peer) to Nesso Watch.
* **Power:** 3.7V LiPo Battery.

---

## 🔌 Wiring & Pinout

### Nesso N1 (Watch)

| Module | Pin on Nesso | Function |
| --- | --- | --- |
| **GSM RX** | Pin 2 | Serial TX (to GSM) |
| **GSM TX** | Pin 7 | Serial RX (from GSM) |
| **GPS RX** | Pin 4 | Serial TX (to GPS) |
| **GPS TX** | N/C | Not Connected |
| **SOS Button** | KEY1 (Library) | Manual Trigger |
| **Power** | Battery Connector | Main Logic Power |

### Seed ESP32 (Dress)

| Component | Pin | Function |
| --- | --- | --- |
| **Velostat** | A0 (ADC) | Pressure Reading |
| **Power** | 3.3V / GND | Power |

---

## 💻 Software Stack

### Firmware (Arduino/C++)

* **IDE:** Arduino IDE 2.0+
* **Core Libraries:**
* `Arduino_Nesso_N1` (Board Control)
* `Arduino_BMI270_BMM150` (IMU)
* `esp_now` & `WiFi` (Communication)
* `TinyGSM` (Cellular)


* **AI Engine:** Edge Impulse (TFLite Micro).

### Web Application

* **Backend:** NestJS (Node.js) - REST API.
* **Frontend:** React (Vite) - Live Dashboard.
* **Database:** (Optional) MongoDB/PostgreSQL for log storage.

---

## ⚙️ Installation & Setup

### 1. Firmware Setup (Watch & Dress)

1. Install **Arduino IDE**.
2. Install the **ESP32 Board Package** (v3.0+).
3. Install required libraries (`Arduino_Nesso_N1`, `EdgeImpulse`, etc.).
4. **Configure Credentials:**
* Open `Config.h` in the Watch firmware.
* Update `WIFI_SSID`, `WIFI_PASS`, and `SERVER_IP`.


5. **Upload:**
* Upload `WBAN_Watch.ino` to the Nesso N1.
* Upload `WBAN_Dress.ino` to the Seed ESP32.



### 2. Web Server Setup

1. Navigate to the `backend` folder:
```bash
cd backend
npm install
npm run start:dev

```


2. Navigate to the `frontend` folder:
```bash
cd frontend
npm install
npm run dev

```


3. Access the dashboard at `http://localhost:5173`.

---

## 📊 How It Works (The Pipeline)

1. **Sensing:** The **Dress** reads pressure values (Velostat).
2. **Transmission:** Dress sends raw data to the **Watch** via ESP-NOW.
3. **Processing:**
* The Watch combines **Dress Data** + **IMU Data**.
* The **AI Model** runs inference (Anomaly Detection).


4. **Decision:**
* If `Panic Score > 0.85` OR `Button Pressed`: **TRIGGER ALARM**.


5. **Alerting:**
* **Try WiFi:** POST JSON data to NestJS Backend.
* **Failover:** If WiFi fails, power up GSM and send **SOS SMS** with Google Maps Link.



---

## 📸 Project Gallery

*(Soon)*

---

## 🔮 Future Scope

* Integration of Heart Rate (PPG) sensors.
* Voice-activated SOS commands.
* Mobile App (React Native) for family members.

---

## 🏆 Team

* **Lead Developer:** Jaswanth
* **Role:** Embedded Systems & Full Stack Integration

---

