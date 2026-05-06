#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "DobleSensor.h" // ¡Incluimos nuestro módulo!

// >>> PON AQUÍ TU IP ACTUAL DE UBUNTU <<<
#define SERVER_IP "172.30.161.239" 
#define SERVER_PORT 5000

typedef struct {
    MuestraSensor lecturas[10];
} PaqueteUDP;

int main() {
    int sockfd;
    struct sockaddr_in server_addr;
    PaqueteUDP paquete;
    
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) exit(EXIT_FAILURE);
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

    // Arrancamos el módulo de sensores
    if (init_sensores() < 0) {
        printf("Error al iniciar el bus I2C\n");
        return 1;
    }

    printf("Muestreo iniciado. Recopilando 10 muestras...\n");

    while(1) {
        for(int i = 0; i < 10; i++) {
            // Llamamos a la función de nuestro módulo directamente
            paquete.lecturas[i] = leer_sensores(); 
            printf("Muestra %d/10 capturada.\n", i+1);
            sleep(1);
        }

        printf("Enviando paquete UDP al servidor...\n");
        sendto(sockfd, &paquete, sizeof(PaqueteUDP), MSG_CONFIRM, (const struct sockaddr *) &server_addr, sizeof(server_addr));
    }
    
    cerrar_sensores();
    close(sockfd);
    return 0;
}