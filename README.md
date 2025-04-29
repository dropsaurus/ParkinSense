# Shake, Rattle, and Roll  
**Embedded Challenge Spring 2025 - Term Project**

## 🧠 Objective
This project aims to detect and classify abnormal hand movements in Parkinson’s patients using data collected from the onboard accelerometer/gyroscope. The primary goals are:
- Detect **tremors** (3–5 Hz): rhythmic oscillations due to low dopamine (“Off” state)
- Detect **dyskinesia** (5–7 Hz): excessive movements caused by excess dopamine (“Too On” state)
- Quantify movement **intensity**
- Indicate conditions using **on-board indicators** (e.g., LEDs, buzzer)

## 📦 Project Features
- 📊 **3-second sampling window** of motion data
- ⚡ **FFT-based frequency analysis** for condition detection
- 🔔 Condition indication using **peripherals only** (no serial output)
- 🔋 Runs on portable power (e.g., power bank)
- 🧩 Implemented entirely on dev board using **PlatformIO**
- 🚫 No external hardware or modules allowed

## 🧪 Detection Criteria
| Symptom      | Frequency Range | Description                         |
|--------------|------------------|-------------------------------------|
| Tremor       | 3–5 Hz           | Indicates “Off” state               |
| Dyskinesia   | 5–7 Hz           | Indicates excessive dopamine levels |

## 🎯 Grading Criteria
| Category                          | Weight |
|----------------------------------|--------|
| Tremor detection                 | 25%    |
| Dyskinesia detection             | 25%    |
| Intensity quantification         | 10%    |
| Robustness (video demonstration) | 10%    |
| Ease of use                      | 10%    |
| Creativity of indication method  | 10%    |
| Code quality                     | 5%     |
| System complexity                | 5%     |
