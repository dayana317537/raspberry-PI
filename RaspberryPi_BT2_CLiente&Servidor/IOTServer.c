#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <math.h> 
#include <stdint.h>

#define PORT 5000
#define MAX_MUESTRAS 100

typedef struct __attribute__((packed)) {
    float ax, ay, az;
    uint16_t r, g, b, c;
} MuestraSensor;

typedef struct __attribute__((packed)) {
    int num_muestras;
    MuestraSensor lecturas[MAX_MUESTRAS];
} PaqueteUDP;

void calcular_stats_float(float datos[], int n, char* nombre) {
    float min = datos[0], max = datos[0], suma = 0.0;
    for(int i=0; i<n; i++) {
        if(datos[i] < min) min = datos[i];
        if(datos[i] > max) max = datos[i];
        suma += datos[i];
    }
    float media = suma / n;
    float sum_var = 0.0;
    for(int i=0; i<n; i++) sum_var += pow(datos[i] - media, 2);
    float desv = sqrt(sum_var / n);
    printf("%s \t-> Media: %6.2f | Min: %6.2f | Max: %6.2f | DesvStd: %6.2f\n", nombre, media, min, max, desv);
}

void calcular_stats_int(uint16_t datos[], int n, char* nombre) {
    uint16_t min = datos[0], max = datos[0];
    float suma = 0.0;
    for(int i=0; i<n; i++) {
        if(datos[i] < min) min = datos[i];
        if(datos[i] > max) max = datos[i];
        suma += datos[i];
    }
    float media = suma / n;
    float sum_var = 0.0;
    for(int i=0; i<n; i++) sum_var += pow(datos[i] - media, 2);
    float desv = sqrt(sum_var / n);
    printf("%s \t-> Media: %6.2f | Min: %6d | Max: %6d | DesvStd: %6.2f\n", nombre, media, min, max, desv);
}

int main(int argc, char *argv[]) {
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    char buffer[4096]; 
    
    int tiempo_a_enviar = 1000; 
    if (argc == 2) {
        tiempo_a_enviar = atoi(argv[1]);
        if (tiempo_a_enviar < 100) tiempo_a_enviar = 100; 
    }
    
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Error socket"); exit(1);
    }
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    bind(sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr));
    
    printf("========================================================\n");
    printf(" Servidor IoT iniciado.\n");
    printf(" Tiempo de muestreo configurado: %d ms.\n", tiempo_a_enviar);
    printf("========================================================\n");
    
    while(1) {
        socklen_t len = sizeof(client_addr);
        int n = recvfrom(sockfd, buffer, sizeof(buffer), MSG_WAITALL, (struct sockaddr *)&client_addr, &len);
        
        if (n == 6 && strncmp(buffer, "CONFIG", 6) == 0) {
            sendto(sockfd, &tiempo_a_enviar, sizeof(tiempo_a_enviar), MSG_CONFIRM, (struct sockaddr *)&client_addr, len);
        }
        else if (n > 6) {
            PaqueteUDP *paquete = (PaqueteUDP *)buffer;
            int n_muestras = paquete->num_muestras;
            
            if (n_muestras > 0 && n_muestras <= MAX_MUESTRAS) {
                printf("\n--- RESULTADOS RECIBIDOS (%d MUESTRAS EN 10s) ---\n", n_muestras);
                
                float ax[MAX_MUESTRAS], ay[MAX_MUESTRAS], az[MAX_MUESTRAS];
                uint16_t r[MAX_MUESTRAS], g[MAX_MUESTRAS], b[MAX_MUESTRAS], c[MAX_MUESTRAS];
                
                for(int i=0; i<n_muestras; i++) {
                    ax[i] = paquete->lecturas[i].ax; 
                    ay[i] = paquete->lecturas[i].ay; 
                    az[i] = paquete->lecturas[i].az;
                    r[i] = paquete->lecturas[i].r; 
                    g[i] = paquete->lecturas[i].g; 
                    b[i] = paquete->lecturas[i].b;
                    c[i] = paquete->lecturas[i].c; // Añadido el Clearance
                }
                
                // El Servidor hace las operaciones y las imprime
                calcular_stats_float(ax, n_muestras, "Aceleracion X");
                calcular_stats_float(ay, n_muestras, "Aceleracion Y");
                calcular_stats_float(az, n_muestras, "Aceleracion Z");
                calcular_stats_int(r, n_muestras, "Color Rojo   ");
                calcular_stats_int(g, n_muestras, "Color Verde  ");
                calcular_stats_int(b, n_muestras, "Color Azul   ");
                calcular_stats_int(c, n_muestras, "Clearance    ");
                
                // Enviar ACK obligatorio
                char ack_msg[] = "ACK: Recibido";
                sendto(sockfd, ack_msg, strlen(ack_msg), MSG_CONFIRM, (struct sockaddr *)&client_addr, len);
            }
        }
    }
    return 0;
}
