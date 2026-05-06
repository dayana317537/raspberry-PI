#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <math.h> // Para la desviación estándar
#include <stdint.h>

#define PORT 5000

// La misma estructura que definimos para el cliente
typedef struct {
    float ax, ay, az;
    uint16_t r, g, b, c;
} MuestraSensor;

typedef struct {
    MuestraSensor lecturas[10];
} PaqueteUDP;

// Función para calcular y mostrar estadísticas de floats (Acelerómetro)
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

// Función para calcular y mostrar estadísticas de enteros (Color)
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

int main() {
    int sockfd;
    PaqueteUDP paquete;
    struct sockaddr_in server_addr, client_addr;
    
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Error socket"); exit(1);
    }
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    
    if (bind(sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error bind"); exit(1);
    }
    
    printf("Servidor IoT listo. Esperando datos de la Raspberry Pi...\n");
    
    while(1) {
        socklen_t len = sizeof(client_addr);
        int n = recvfrom(sockfd, &paquete, sizeof(PaqueteUDP), MSG_WAITALL, (struct sockaddr *)&client_addr, &len);
        
        if (n == sizeof(PaqueteUDP)) {
            printf("\n--- NUEVO PAQUETE DE 10 SEGUNDOS RECIBIDO ---\n");
            float ax[10], ay[10], az[10];
            uint16_t r[10], g[10], b[10];
            
            for(int i=0; i<10; i++) {
                ax[i] = paquete.lecturas[i].ax; ay[i] = paquete.lecturas[i].ay; az[i] = paquete.lecturas[i].az;
                r[i] = paquete.lecturas[i].r; g[i] = paquete.lecturas[i].g; b[i] = paquete.lecturas[i].b;
            }
            
            calcular_stats_float(ax, 10, "Aceleracion X");
            calcular_stats_float(ay, 10, "Aceleracion Y");
            calcular_stats_float(az, 10, "Aceleracion Z");
            calcular_stats_int(r, 10, "Color Rojo   ");
            calcular_stats_int(g, 10, "Color Verde  ");
            calcular_stats_int(b, 10, "Color Azul   ");
            
            // Enviar respuesta al cliente para que sepa que todo ok
            char *ack = "ACK: Datos recibidos";
            sendto(sockfd, ack, strlen(ack), 0, (struct sockaddr *)&client_addr, len);
        }
    }
    return 0;
}