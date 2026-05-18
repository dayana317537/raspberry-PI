#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <stdint.h>

#define SERVER_IP "172.30.161.122" // Asegúrate de que esta sea la IP de tu máquina virtual
#define SERVER_PORT 5000
#define MAX_MUESTRAS 100

#define MPU_ADDR 0x68   
#define REG_ACCEL 0x3B  
#define COLOR_ADR 0x29
#define CMD 0x80
#define REG_ENABLE 0x00
#define REG_ATIME  0x01
#define REF_DATOS_COLOR 0x14 

typedef struct __attribute__((packed)) {
    float ax, ay, az;
    uint16_t r, g, b, c;
} MuestraSensor;

typedef struct __attribute__((packed)) {
    int num_muestras;
    MuestraSensor lecturas[MAX_MUESTRAS];
} PaqueteUDP;

int i2c_fd;

void init_sensores() {
    i2c_fd = open("/dev/i2c-1", O_RDWR);
    if (i2c_fd < 0) { perror("Error I2C"); exit(1); }

    ioctl(i2c_fd, I2C_SLAVE, MPU_ADDR);
    unsigned char cfg_accel[2] = {0x6B, 0x00};
    write(i2c_fd, cfg_accel, 2);

    ioctl(i2c_fd, I2C_SLAVE, COLOR_ADR);
    unsigned char cfg_color_en[2] = {CMD | REG_ENABLE, 0x03};
    write(i2c_fd, cfg_color_en, 2);
    unsigned char cfg_color_time[2] = {CMD | REG_ATIME, 0xEB}; 
    write(i2c_fd, cfg_color_time, 2);
}

void leer_sensores(MuestraSensor *m) {
    ioctl(i2c_fd, I2C_SLAVE, MPU_ADDR);
    unsigned char reg_accel = REG_ACCEL;
    write(i2c_fd, &reg_accel, 1);
    unsigned char buf_accel[6];
    read(i2c_fd, buf_accel, 6);
    
    int16_t raw_ax = (buf_accel[0] << 8) | buf_accel[1];
    int16_t raw_ay = (buf_accel[2] << 8) | buf_accel[3];
    int16_t raw_az = (buf_accel[4] << 8) | buf_accel[5];
    
    m->ax = raw_ax / 16384.0;
    m->ay = raw_ay / 16384.0;
    m->az = raw_az / 16384.0;

    ioctl(i2c_fd, I2C_SLAVE, COLOR_ADR);
    unsigned char reg_color = CMD | REF_DATOS_COLOR;
    write(i2c_fd, &reg_color, 1);
    unsigned char buf_color[8];
    read(i2c_fd, buf_color, 8);
    
    m->c = (buf_color[1] << 8) | buf_color[0];
    m->r = (buf_color[3] << 8) | buf_color[2];
    m->g = (buf_color[5] << 8) | buf_color[4];
    m->b = (buf_color[7] << 8) | buf_color[6];
}

int main() {
    int sockfd;
    struct sockaddr_in server_addr;
    PaqueteUDP paquete;
    int tiempo_ms = 1000; 

    init_sensores();

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

    printf("Pidiendo configuración al servidor...\n");
    sendto(sockfd, "CONFIG", 6, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
    
    struct timeval tv;
    tv.tv_sec = 2; tv.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
    
    if (recvfrom(sockfd, &tiempo_ms, sizeof(int), 0, NULL, NULL) > 0) {
        printf("Configuración recibida: Medir cada %d ms.\n\n", tiempo_ms);
    } else {
        printf("Sin respuesta. Usando 1000ms.\n\n");
        tiempo_ms = 1000;
    }

    int n_muestras = 10000 / tiempo_ms;
    if (n_muestras > MAX_MUESTRAS) n_muestras = MAX_MUESTRAS;
    if (n_muestras <= 0) n_muestras = 1;

    while(1) {
        paquete.num_muestras = n_muestras;
        
        for(int i = 0; i < n_muestras; i++) {
            leer_sensores(&paquete.lecturas[i]);
            // Impresión limpia, tal y como pedías
            printf("Muestra %d/%d tomada.\n", i+1, n_muestras);
            usleep(tiempo_ms * 1000);
        }
        
        printf("--- Bloque completado. Enviando al servidor... ---\n\n");
        sendto(sockfd, &paquete, sizeof(PaqueteUDP), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
    }
    
    close(i2c_fd); close(sockfd);
    return 0;
}
