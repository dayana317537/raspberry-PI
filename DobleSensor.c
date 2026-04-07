#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <unistd.h>
#include <stdint.h>

// --- DEFINICIONES MPU-6050 (Acelerómetro) ---
#define MPU_ADDR 0x68         
#define REG_PWR_MGMT_1 0x6B    
#define REG_ACCEL_XOUT_H 0x3B  

// --- DEFINICIONES Flora (Sensor de Color) ---
#define FLORA_ADDR 0x29
#define CMD 0x80
#define REG_ENABLE 0x00
#define REG_ATIME  0x01
#define REG_CDATAL 0x14 

int main() {
    printf("Iniciando Sistema Dual I2C (MPU6050 + Flora)...\n");

    // ---------------------------------------------------------
    // 1. APERTURA DEL BUS (Doble Descriptor)
    // ---------------------------------------------------------
    char *bus = "/dev/i2c-1";
    int fd_accel = open(bus, O_RDWR);
    int fd_color = open(bus, O_RDWR);

    if (fd_accel < 0 || fd_color < 0) {
        perror("Error crítico: No se pudo abrir el bus I2C");
        return 1;
    }

    // Enlazar el descriptor del color a su dirección (0x29)
    if (ioctl(fd_color, I2C_SLAVE, FLORA_ADDR) < 0) {
        perror("Error: No se encontró Flora");
        return 1;
    }

    // ---------------------------------------------------------
    // 2. INICIALIZACIÓN (Despertar a ambos sensores)
    // ---------------------------------------------------------
    
    // -> Despertar MPU-6050 (Usando tu estructura ioctl)
    struct i2c_rdwr_ioctl_data packets;
    struct i2c_msg messages[2];
    uint8_t write_accel[2] = {REG_PWR_MGMT_1, 0x00};
    
    messages[0].addr = MPU_ADDR;
    messages[0].flags = 0;
    messages[0].len = 2;
    messages[0].buf = write_accel;
    packets.msgs = messages;
    packets.nmsgs = 1;
    ioctl(fd_accel, I2C_RDWR, &packets);

    // -> Despertar Flora
    uint8_t init_color[2] = {CMD | REG_ENABLE, 0x03};
    write(fd_color, init_color, 2);
    
    uint8_t timing[2] = {CMD | REG_ATIME, 0xEB}; // ~60ms
    write(fd_color, timing, 2);

    printf("Sensores activados. Entrando en bucle de lectura...\n\n");

    // Variables para lectura
    uint8_t read_accel[14];
    uint8_t read_color[8];

    // ---------------------------------------------------------
    // 3. BUCLE PRINCIPAL (Super Loop)
    // ---------------------------------------------------------
    while(1) {
        
        // --- A. LEER ACELERÓMETRO ---
        write_accel[0] = REG_ACCEL_XOUT_H; // Puntero
        
        messages[0].addr = MPU_ADDR;
        messages[0].flags = 0;
        messages[0].len = 1;
        messages[0].buf = write_accel;

        messages[1].addr = MPU_ADDR;
        messages[1].flags = I2C_M_RD;
        messages[1].len = 14;
        messages[1].buf = read_accel;

        packets.msgs = messages;
        packets.nmsgs = 2;
        ioctl(fd_accel, I2C_RDWR, &packets);

        // Procesar Acelerómetro
        int16_t raw_ax = (read_accel[0] << 8) | read_accel[1];
        int16_t raw_ay = (read_accel[2] << 8) | read_accel[3];
        int16_t raw_az = (read_accel[4] << 8) | read_accel[5];
        float ax = raw_ax / 16384.0;
        float ay = raw_ay / 16384.0;
        float az = raw_az / 16384.0;

        // --- B. LEER COLOR ---
        uint8_t reg_ptr = CMD | REG_CDATAL;
        write(fd_color, &reg_ptr, 1);
        read(fd_color, read_color, 8);

        // Procesar Color
        uint16_t clear = (read_color[1] << 8) | read_color[0];
        uint16_t red   = (read_color[3] << 8) | read_color[2];
        uint16_t green = (read_color[5] << 8) | read_color[4];
        uint16_t blue  = (read_color[7] << 8) | read_color[6];

        // --- C. IMPRIMIR DATOS UNIFICADOS ---
        printf("\rACCEL [g]: X=%5.2f Y=%5.2f Z=%5.2f | COLOR: R=%5d G=%5d B=%5d C=%5d", 
                ax, ay, az, red, green, blue, clear);
        fflush(stdout);

        // --- D. CONTROL DE FRECUENCIA ---
        usleep(200000); // 200ms (5Hz)
    }

    // 4. CIERRE SEGURO (Teórico, el while es infinito)
    close(fd_accel);
    close(fd_color);
    return 0;
}