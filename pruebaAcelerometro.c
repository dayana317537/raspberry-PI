#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <unistd.h>
#include <stdint.h>

// Definiciones del MPU-6050
#define MPU_ADDR 0x68         // Dirección del registro del sensor por comunicacion I2C, verificar con I2Cdetect -y 1
#define REG_PWR_MGMT_1 0x6B    // Registro del control de energia, el sensor esta en SLEEP al principio, enviando el 6B y 0x00 se despierta al sensor y se establece que funcione a una frecuencia de 8MHz
#define REG_ACCEL_XOUT_H 0x3B  // Inicio datos acelerómetro, es decir, establece el modo de funcionamiento a acelerometro. Si le sigue un 0x00, son 16 bits en complemento a2 y se almacena la medida mas reciente del eje x
#define REG_GYRO_XOUT_H 0x43   // Inicio datos giroscopio, lo mismo que la anterior, registro de solo lectura del giroscopio que almacena 16 bits en CA2 con el ultimo valor de x, si es 0x00 ± 250 °/s y 131 LSB/°/s

int main(int argc, char *argv[]) {
    // 1. System Initialization: Read parameters
    //int dev = 1; // Por defecto bus 1
    //if(argc > 1) dev = atoi(argv[1]);

    printf("Prueba de acelerometro MPU-6050\n");

    char i2cFile[11]= "/dev/i2c-1";
    //sprintf(i2cFile, "/dev/i2c-1");
    
    // Abrir y configurar sensor
    int fd = open(i2cFile, O_RDWR);
    if (fd < 0) {
        perror("Error al abrir bus I2C");
        return 1;
    }

    // Estructuras para mensajes I2C (Esqueleto solicitado)
    struct i2c_rdwr_ioctl_data packets;
    struct i2c_msg messages[2];
    uint8_t write_bytes[2];
    uint8_t read_bytes[14]; // 6 para Accel, 2 para Temp, 6 para Gyro

    // 2. System Operation: Activate the sensor
    // Despertar el sensor (0x00 a PWR_MGMT_1)
    write_bytes[0] = REG_PWR_MGMT_1;
    write_bytes[1] = 0x00; 

    messages[0].addr = MPU_ADDR;
    messages[0].flags = 0; // Write
    messages[0].len = 2;
    messages[0].buf = write_bytes;

    packets.msgs = messages;
    packets.nmsgs = 1;
    
    if (ioctl(fd, I2C_RDWR, &packets) < 0) {
        perror("Error al activar el sensor");
        return 1;
    }

    printf("Sensor activado. Leyendo datos... (Ctrl+C para salir)\n");

    // 3. Loop through sensor data sample
    while(1) {
        // Preparar lectura de ráfaga (Escribir dirección inicial + Leer 14 bytes)
        write_bytes[0] = REG_ACCEL_XOUT_H; // Empezamos en 0x3B

        // Mensaje 0: Escribir la dirección del registro que queremos leer
        messages[0].addr = MPU_ADDR;
        messages[0].flags = 0;
        messages[0].len = 1;
        messages[0].buf = write_bytes;

        // Mensaje 1: Leer los datos resultantes
        messages[1].addr = MPU_ADDR;
        messages[1].flags = I2C_M_RD;
        messages[1].len = 14; 
        messages[1].buf = read_bytes;

        packets.msgs = messages;
        packets.nmsgs = 2; // Operación combinada Write+Read

        if (ioctl(fd, I2C_RDWR, &packets) < 0) {
            perror("Error en muestreo");
            break;
        }

        // 4. Transform data into engineering units
        // Combinar bytes (High << 8 | Low)
        int16_t raw_ax = (read_bytes[0] << 8) | read_bytes[1];
        int16_t raw_ay = (read_bytes[2] << 8) | read_bytes[3];
        int16_t raw_az = (read_bytes[4] << 8) | read_bytes[5];
        
        int16_t raw_gx = (read_bytes[8] << 8) | read_bytes[9];
        int16_t raw_gy = (read_bytes[10] << 8) | read_bytes[11];
        int16_t raw_gz = (read_bytes[12] << 8) | read_bytes[13];

        // Conversión según sensibilidad ±2g (16384 LSB/g) y ±250°/s (131 LSB/°/s)
        float ax = raw_ax / 16384.0;
        float ay = raw_ay / 16384.0;
        float az = raw_az / 16384.0;
        
        float gx = raw_gx / 131.0;
        float gy = raw_gy / 131.0;
        float gz = raw_gz / 131.0;

        // Mostrar datos
        printf("\rACCEL [g]: X=%.2f Y=%.2f Z=%.2f | GYRO [°/s]: X=%.2f Y=%.2f Z=%.2f", 
                ax, ay, az, gx, gy, gz);
        fflush(stdout);

        // 5. Control sample frequency (10Hz)
        usleep(100000); 
    }

    // 6. System Closure
    printf("\nCerrando sistema...\n");
    close(fd);
    return 0;
}