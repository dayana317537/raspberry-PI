#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <stdint.h>

#define SERVER_IP "172.30.161.122" 
#define SERVER_PORT 5000
#define MAX_MUESTRAS 100

// TODO: Fase 2 - Definir direcciones I2C de los sensores MPU y Color
// #define MPU_ADDR 0x68   
// ...

typedef struct __attribute__((packed)) {
    float ax, ay, az;
    uint16_t r, g, b, c;
} MuestraSensor;

typedef struct __attribute__((packed)) {
    int num_muestras;
    MuestraSensor lecturas[MAX_MUESTRAS];
} PaqueteUDP;

// int i2c_fd;

void init_sensores() {
    // TODO: Fase 2 - Abrir puerto /dev/i2c-1 y configurar registros del acelerómetro y sensor de color.
    printf("[INFO] Inicialización de sensores hardware pendiente (Fase 2).\n");
}

void leer_sensores(MuestraSensor *m) {
    // Falta implementar ioctl() y read() para obtener datos reales del bus I2C.
    // Por ahora, rellenamos con datos de prueba para probar la comunicación con el servidor.
    m->ax = 0.05; 
    m->ay = -0.02; 
    m->az = 0.98;
    
    m->c = 1500;
    m->r = 250;
    m->g = 120;
    m->b = 80;
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
            printf("Muestra %d/%d tomada (Dummy).\n", i+1, n_muestras);
            usleep(tiempo_ms * 1000);
        }
        
        printf("--- Bloque completado. Enviando al servidor... ---\n\n");
        sendto(sockfd, &paquete, sizeof(PaqueteUDP), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
    }
    
    close(sockfd);
    // close(i2c_fd); // Pendiente para la fase 2
    return 0;
}