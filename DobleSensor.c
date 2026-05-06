#include "DobleSensor.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <unistd.h>

#define MPU_ADDR 0x68   
#define REG_ACCEL 0x3B  
#define COLOR_ADR 0x29
#define CMD 0x80
#define REG_ENABLE 0x00
#define REG_ATIME  0x01
#define REF_DATOS_COLOR 0x14 

int fd_accel;
int fd_color;

int init_sensores(void) {
    char *bus = "/dev/i2c-1";
    fd_accel = open(bus, O_RDWR);
    fd_color = open(bus, O_RDWR);

    if (fd_accel < 0 || fd_color < 0) return -1;

    // Iniciar Acelerómetro
    struct i2c_rdwr_ioctl_data datos_i2c;
    struct i2c_msg messages[1];
    uint8_t write_accel[2] = {0x6B, 0x00};
    messages[0].addr = MPU_ADDR; messages[0].flags = 0; messages[0].len = 2; messages[0].buf = write_accel;
    datos_i2c.msgs = messages; datos_i2c.nmsgs = 1;
    ioctl(fd_accel, I2C_RDWR, &datos_i2c);

    // Iniciar Color
    ioctl(fd_color, I2C_SLAVE, COLOR_ADR);
    uint8_t init_color[2] = {CMD | REG_ENABLE, 0x03};
    write(fd_color, init_color, 2);
    uint8_t timing[2] = {CMD | REG_ATIME, 0xEB};
    write(fd_color, timing, 2);

    return 0;
}

MuestraSensor leer_sensores(void) {
    MuestraSensor m;
    struct i2c_rdwr_ioctl_data datos_i2c;
    struct i2c_msg messages[2];
    uint8_t write_accel[1] = {REG_ACCEL};
    uint8_t read_accel[14];
    uint8_t read_color[8];

    // Leer Acelerómetro
    messages[0].addr = MPU_ADDR; messages[0].flags = 0; messages[0].len = 1; messages[0].buf = write_accel;
    messages[1].addr = MPU_ADDR; messages[1].flags = I2C_M_RD; messages[1].len = 14; messages[1].buf = read_accel;
    datos_i2c.msgs = messages; datos_i2c.nmsgs = 2;
    ioctl(fd_accel, I2C_RDWR, &datos_i2c);

    int16_t raw_ax = (read_accel[0] << 8) | read_accel[1];
    int16_t raw_ay = (read_accel[2] << 8) | read_accel[3];
    int16_t raw_az = (read_accel[4] << 8) | read_accel[5];
    m.ax = raw_ax / 16384.0; m.ay = raw_ay / 16384.0; m.az = raw_az / 16384.0;

    // Leer Color
    uint8_t reg_color = CMD | REF_DATOS_COLOR;
    write(fd_color, &reg_color, 1);
    read(fd_color, read_color, 8);
    m.c = (read_color[1] << 8) | read_color[0];
    m.r = (read_color[3] << 8) | read_color[2];
    m.g = (read_color[5] << 8) | read_color[4];
    m.b = (read_color[7] << 8) | read_color[6];

    return m;
}

void cerrar_sensores(void) {
    close(fd_accel); close(fd_color);
}