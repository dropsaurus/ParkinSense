#include "mbed.h"
#include "lsm6dsl_reg.h"
#include "arm_math.h"

#define ARM_MATH_CM4
#define __FPU_PRESENT 1

/*
 * This program is designed for the STM32L4 Discovery IoT Node board, using the LSM6DSL 3D accelerometer.
 * 
 * Functionality Overview:
 * - Continuously reads live accelerometer data via I2C from the LSM6DSL sensor.
 * - Collects 256 samples of 3-axis acceleration, calculates the magnitude of movement.
 * - Performs FFT (Fast Fourier Transform) on the collected data to analyze frequency components.
 * - Based on the frequency and amplitude of motion, it detects two types of movement disorders:
 *     - Tremor: characterized by strong frequency components in the 3–5 Hz range.
 *     - Dyskinesia: characterized by frequencies in the 5–7 Hz range.
 * - Uses an LED (PB_14) to indicate detection:
 *     - Solid ON for tremor.
 *     - Blinking for dyskinesia.
 * - Uses another LED (PA_5) to indicate data collection is in progress.
 * - The sampling rate is configured to 104Hz, and detection is repeated every 5 seconds.
 */

DigitalOut led_tremor(PB_14);     // LED for tremor detection
DigitalOut led_detecting(PA_5);   // LED for data collection



// I2C & Serial
I2C i2c(PB_11, PB_10);     // I2C2: SDA = PB11, SCL = PB10
BufferedSerial pc(USBTX, USBRX, 115200);

// FileHandle *mbed::mbed_override_console(int) {
//     return &pc;
// }

// LSM6DSL
#define LSM6DSL_I2C_ADDR  0x6A  // 7-bit I2C address
stmdev_ctx_t dev_ctx;

// Generic I2C read/write functions
int32_t platform_write(void *handle, uint8_t reg, const uint8_t *bufp, uint16_t len) {
    char data[17];
    if (len > 16) return -1;
    data[0] = reg;
    memcpy(&data[1], bufp, len);
    return i2c.write(((int)(intptr_t)handle << 1), data, len + 1) == 0 ? 0 : -1;
}

int32_t platform_read(void *handle, uint8_t reg, uint8_t *bufp, uint16_t len) {
    char reg_addr = reg;
    if (i2c.write(((int)(intptr_t)handle << 1), &reg_addr, 1, true) != 0) return -1;
    return i2c.read(((int)(intptr_t)handle << 1), (char*)bufp, len) == 0 ? 0 : -1;
}

void analyze_motion(const float* magnitudes, int sample_size, float sampling_rate);

void test_fft_accelerometer() {
    constexpr int SAMPLE_SIZE = 256;
    // float32_t accel_z[SAMPLE_SIZE];
    float32_t accel_magnitude[SAMPLE_SIZE];
    float32_t output[2 * SAMPLE_SIZE];
    int16_t data_raw[3];

    printf("Initializing FFT on live accelerometer data...\n");

    // Setup sensor context
    dev_ctx.write_reg = platform_write; // I2C write function
    dev_ctx.read_reg = platform_read;   // I2C read function
    dev_ctx.handle = (void*)LSM6DSL_I2C_ADDR;   // I2C address

    thread_sleep_for(500);   // Wait for the sensor to stabilize

    uint8_t whoami = 0;  // used to check device ID
    if (lsm6dsl_device_id_get(&dev_ctx, &whoami) != 0 || whoami != 0x6A) {
        printf("Device not found or ID mismatch. WHO_AM_I = 0x%X\n", whoami);
        return;
    }

    lsm6dsl_reset_set(&dev_ctx, PROPERTY_ENABLE);
    uint8_t rst;
    do {
        lsm6dsl_reset_get(&dev_ctx, &rst);
    } while (rst);

    lsm6dsl_xl_data_rate_set(&dev_ctx, LSM6DSL_XL_ODR_104Hz);
    lsm6dsl_xl_full_scale_set(&dev_ctx, LSM6DSL_2g);

    // Collect SAMPLE_SIZE samples of Z-axis data
    printf("Collecting %d samples...\n", SAMPLE_SIZE);  // Start collecting data
    led_detecting = 1;
    int collected = 0;
    while (collected < SAMPLE_SIZE) {
        lsm6dsl_status_reg_t status;
        lsm6dsl_status_reg_get(&dev_ctx, &status);
        if (status.xlda) {
            lsm6dsl_acceleration_raw_get(&dev_ctx, data_raw);
            // accel_z[collected] = lsm6dsl_from_fs2g_to_mg(data_raw[2]) / 1000.0f;
            float x = lsm6dsl_from_fs2g_to_mg(data_raw[0]) / 1000.0f;
            float y = lsm6dsl_from_fs2g_to_mg(data_raw[1]) / 1000.0f;
            float z = lsm6dsl_from_fs2g_to_mg(data_raw[2]) / 1000.0f;
            accel_magnitude[collected] = sqrtf(x * x + y * y + z * z);
            collected++;
        }
        thread_sleep_for(10);
    }
    led_detecting = 0;

    // Perform FFT
    arm_rfft_fast_instance_f32 fft_instance;
    if (arm_rfft_fast_init_f32(&fft_instance, SAMPLE_SIZE) != ARM_MATH_SUCCESS) {
        printf("FFT initialization failed.\n");
        return;
    }

    arm_rfft_fast_f32(&fft_instance, accel_magnitude, output, 0);

    printf("FFT Magnitudes (first half):\n");
    for (int i = 0; i < SAMPLE_SIZE / 2; i++) {
        float real = output[2 * i];
        float imag = output[2 * i + 1];
        float magnitude = sqrtf(real * real + imag * imag);
        int mag_int = (int)(magnitude * 1000);
        printf("Bin %d: %d.%03d\n", i, mag_int / 1000, mag_int % 1000);
    }


    float magnitudes[SAMPLE_SIZE / 2];
    for (int i = 0; i < SAMPLE_SIZE / 2; i++) {
        float real = output[2 * i];
        float imag = output[2 * i + 1];
        magnitudes[i] = sqrtf(real * real + imag * imag);
    }
    analyze_motion(magnitudes, SAMPLE_SIZE, 104.0f); 
}

/*
 * This program collects accelerometer data from the LSM6DSL sensor to analyze motion patterns and detect abnormalities.
 * The process involves the following steps:
 * 
 * 1. **Data Collection**:
 *    - The accelerometer is configured to sample data at a fixed rate of 104 Hz.
 *    - A total of 256 samples are collected, representing approximately 2.46 seconds of motion data.
 *    - For each sample, the magnitude of acceleration is calculated as sqrt(x^2 + y^2 + z^2), combining the X, Y, and Z axis data.
 * 
 * 2. **Frequency Analysis**:
 *    - The collected data is processed using a Fast Fourier Transform (FFT) to convert the time-domain signal into the frequency domain.
 *    - Only the first half of the FFT output is analyzed, as the second half is symmetric for real-valued input signals.
 *    - The frequency resolution is calculated as `frequency_resolution = sampling_rate / sample_size`, allowing each FFT bin to be mapped to a specific frequency.
 * 
 * 3. **Motion Detection**:
 *    - The program identifies two types of motion abnormalities based on frequency and amplitude thresholds:
 *      - **Tremor**: Detected in the 3–5 Hz frequency range if the amplitude exceeds 14.0 and at least 2 bins meet this condition.
 *      - **Dyskinesia**: Detected in the 5–7 Hz frequency range if the amplitude exceeds 15.0 and at least 3 bins meet this condition.
 *    - If tremor is detected, the LED remains solid ON for 5 seconds. If dyskinesia is detected, the LED blinks 25 times with a 200 ms interval.
 *    - If no abnormal motion is detected, the LED remains OFF.
 * 
 * 4. **Real-Time Operation**:
 *    - The program continuously repeats the detection process every 5 seconds, providing real-time feedback on motion patterns.
 */

void analyze_motion(const float* magnitudes, int sample_size, float sampling_rate) {
    float frequency_resolution = sampling_rate / sample_size;

    // Thresholds (tunable based on real-world tests)
    float tremor_threshold = 14.0f;
    float dyskinesia_threshold = 15.0f;
    int tremor_count = 0;
    int dyskinesia_count = 0;

    for (int i = 1; i < sample_size / 2; i++) {
        float freq = i * frequency_resolution;
        float amp = magnitudes[i];

        if (freq >= 3.0f && freq <= 5.0f && amp >= tremor_threshold) {   // 3-5 Hz
            tremor_count++;
        } else if (freq > 5.0f && freq <= 7.0f && amp >= dyskinesia_threshold) {    // 5-7 Hz
            dyskinesia_count++;
        }
    }

    if (tremor_count >= 2) {
        printf("Tremor Detected\n");
        led_tremor = 1;
        thread_sleep_for(5000);
        led_tremor = 0;     
    } else if (dyskinesia_count >= 3) {
        printf("Dyskinesia Detected\n");
        for (int i = 0; i < 25; i++) {
            led_tremor = !led_tremor;
            thread_sleep_for(200);
        }
    } else {
        printf("No abnormal motion detected\n");
        led_tremor = 0;
    }
}



int main() {
    thread_sleep_for(1000);
    printf("FFT Start\n");

    while (1) {
        test_fft_accelerometer();          
        thread_sleep_for(5000);             // test every 5 seconds
    }
}