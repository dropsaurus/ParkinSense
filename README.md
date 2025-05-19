# ParkinSense 

## 🧠 Objective
This project aims to detect and classify abnormal hand movements in Parkinson’s patients using data collected from the onboard accelerometer/gyroscope. The primary goals are:
- Detect **tremors** (3–5 Hz): rhythmic oscillations due to low dopamine (“Off” state)
- Detect **dyskinesia** (5–7 Hz): excessive movements caused by excess dopamine (“Too On” state)
- Quantify movement **intensity**
- Indicate conditions using **on-board indicators** (e.g., LEDs)

## 📦 Project Features
- 📊 **3-second sampling window** of motion data
- ⚡ **FFT-based frequency analysis** for condition detection
- 🔔 Condition indication using **onboard LEDs**
- 🌟 **Intensity quantification**: Yellow LED (PC_9) lights up when motion intensity exceeds a threshold
- 🔋 Runs on portable power (e.g., power bank)
- 🧩 Implemented entirely on dev board using **PlatformIO**
- 🚫 No external hardware or modules required

## 🧪 Detection Criteria
| Symptom      | Frequency Range | Description                         |
|--------------|-----------------|-------------------------------------|
| Tremor       | 3–5 Hz          | Indicates “Off” state               |
| Dyskinesia   | 5–7 Hz          | Indicates excessive dopamine levels |
| Intensity    | Peak Amplitude  | Indicates strong motion intensity   |

## ⚙️ How It Works

### 1. **Data Collection**
- The onboard accelerometer samples motion data at a fixed rate of **104 Hz**.
- A **circular buffer** stores the last **312 samples** (approximately 3 seconds of data).
- For each sample, the **magnitude of acceleration** is calculated as `sqrt(x^2 + y^2 + z^2)` to combine the X, Y, and Z axis data.

### 2. **Frequency Analysis**
- A **Fast Fourier Transform (FFT)** is performed on the most recent **256 samples** from the circular buffer.
- The FFT converts the time-domain signal into the frequency domain, allowing analysis of motion frequencies.
- Only the **first half** of the FFT output is analyzed, as the second half is symmetric for real-valued input signals.
- The **frequency resolution** is calculated as `frequency_resolution = sampling_rate / sample_size`, allowing each FFT bin to correspond to a specific frequency.

### 3. **Motion Detection**
- The program identifies two types of motion abnormalities based on frequency and amplitude thresholds:
  - **Tremor**: Detected in the **3–5 Hz** frequency range if the amplitude exceeds **14.0** and at least **2 bins** meet this condition.
  - **Dyskinesia**: Detected in the **5–7 Hz** frequency range if the amplitude exceeds **15.0** and at least **3 bins** meet this condition.

### 4. **Intensity Quantification**
- The system calculates the **peak amplitude** in both the Tremor (3–5 Hz) and Dyskinesia (5–7 Hz) frequency ranges.
- If the peak amplitude exceeds a predefined threshold (e.g., **100.0**), the **yellow LED (PC_9)** is turned ON to indicate strong motion intensity.
- This feature provides real-time feedback on the severity of the detected symptoms.

### 5. **Condition Indication**
- Detection results are indicated using onboard LEDs:
  - **Tremor**: Only the **PB_14 LED** is turned ON.
  - **Dyskinesia**: Both **PB_14 LED** and **PA_5 LED** are turned ON.
  - **Strong Motion Intensity**: The **PC_9 LED** (yellow) is turned ON when the motion intensity exceeds the threshold.
  - **No abnormal motion**: All LEDs are turned OFF.

### 6. **Real-Time Operation**
- The detection process is repeated continuously, with a short delay of **10 ms** between iterations to maintain real-time responsiveness.
- The system operates entirely on the embedded platform, with no reliance on external hardware or serial output for condition indication.

### 7. **Team Members**
- Abby Zhang
- Moulik Shah
- Neil Noronha
- Mike Zeng
- Dongting
