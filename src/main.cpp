#include "mbed.h"
#include "lsm6dsl_reg.h"

// I2C & Serial
I2C i2c(PB_11, PB_10);
BufferedSerial pc(USBTX, USBRX, 115200);

FileHandle *mbed::mbed_override_console(int) {
    return &pc;
}

// LSM6DSL
#define LSM6DSL_I2C_ADDR  0x6A  // 7-bit 
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

int main() {
    printf("Initialization started\n");

    dev_ctx.write_reg = platform_write;
    dev_ctx.read_reg = platform_read;
    dev_ctx.handle = (void*)LSM6DSL_I2C_ADDR;

    thread_sleep_for(500);

    printf("Preparing to read WHO_AM_I…\n");
    uint8_t whoami = 0;
    int result = lsm6dsl_device_id_get(&dev_ctx, &whoami);
    printf("device_id_get() Retrun Value: %d\n", result);
    printf("WHO_AM_I: 0x%X\n", whoami);

    if (lsm6dsl_device_id_get(&dev_ctx, &whoami) != 0) {
        printf("Read WHO_AM_I Failed\n");
        return 1;
    }


    if (whoami != 0x6A) {
        printf("Device ID mismatch, I2C may not be connected\n");
        return 1;
    }

    // Normal initialization process
    printf("Device connected successfully, starting reset...\n");
    fflush(stdout);
    lsm6dsl_reset_set(&dev_ctx, PROPERTY_ENABLE);
    uint8_t rst;
    do {
        lsm6dsl_reset_get(&dev_ctx, &rst);
    } while (rst);
    printf("Reset completed\n");

    lsm6dsl_xl_data_rate_set(&dev_ctx, LSM6DSL_XL_ODR_104Hz);
    lsm6dsl_xl_full_scale_set(&dev_ctx, LSM6DSL_2g);
    printf("Accelerometer configuration completed\n");

    while (true) {
        lsm6dsl_status_reg_t status;
        lsm6dsl_status_reg_get(&dev_ctx, &status);

        static float prev_magnitude = 1.0f;
        if (status.xlda) {
            int16_t data_raw[3];
            float accel_mg[3];

            lsm6dsl_acceleration_raw_get(&dev_ctx, data_raw);

            for (int i = 0; i < 3; i++) {
                accel_mg[i] = lsm6dsl_from_fs2g_to_mg(data_raw[i]) / 1000.0f;
            }

            float magnitude = sqrtf(accel_mg[0]*accel_mg[0] + accel_mg[1]*accel_mg[1] + accel_mg[2]*accel_mg[2]);

            if (fabs(magnitude - prev_magnitude) > 0.05f) {
                int int_mag = (int)(magnitude * 100);
                printf("Current acceleration magnitude: %d.%02d g\n", int_mag / 100, int_mag % 100);
                prev_magnitude = magnitude;
            }
        }

        thread_sleep_for(100);
    }
}