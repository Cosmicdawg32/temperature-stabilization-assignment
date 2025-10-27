#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <math.h>           // for fabs()
#include "utils.h"          // struct msg and EPS

#define numExternals 4
#define PORT 2000

// This is a stub for your connection function
// Make sure you already have this implemented elsewhere in your codebase.
int * establishConnectionsFromExternalProcesses();

int main(void)
{
    int socket_desc; 
    struct msg messageFromClient;   

    // Establish client connections and return 
    // an array of file descriptors of client sockets. 
    int * client_socket = establishConnectionsFromExternalProcesses(); 

    // Stabilization variables
    int stable = 0;
    float centralTemp = 100.0f;  // can also set from argv if desired

    while (!stable) {
        float temperature[numExternals];

        // STEP 1: Receive temps from all external processes
        for (int i = 0; i < numExternals; i++) {
            if (recv(client_socket[i], (void *)&messageFromClient, sizeof(messageFromClient), 0) <= 0) {
                printf("Couldn't receive from client %d\n", i);
                return -1;
            }
            temperature[i] = messageFromClient.T;
            printf("Temperature of External Process (%d) = %f\n", i, temperature[i]);
        }

        // STEP 2: Update central temperature
        float oldCentralTemp = centralTemp;
        float sumExternal = 0.0f;
        for (int i = 0; i < numExternals; i++) {
            sumExternal += temperature[i];
        }

        centralTemp = (2.0f * centralTemp + sumExternal) / 6.0f;

        // STEP 3: Check stabilization
        int doneFlag = (fabs(centralTemp - oldCentralTemp) < EPS) ? 1 : 0;

        // STEP 4: Broadcast new temperature to clients
        struct msg updated_msg;
        updated_msg.T = centralTemp;
        updated_msg.Index = 0;
        updated_msg.Done = doneFlag;

        for (int i = 0; i < numExternals; i++) {
            if (send(client_socket[i], (const void *)&updated_msg, sizeof(updated_msg), 0) < 0) {
                printf("Can't send to client %d\n", i);
                return -1;
            }
        }

        printf("Central temperature updated to: %f\n\n", centralTemp);

        if (doneFlag) {
            stable = 1;
        }
    }

    printf("✅ System stabilized at %.4f °C\n", centralTemp);

    // STEP 5: Clean up sockets
    for (int i = 0; i < numExternals; i++) {
        close(client_socket[i]);
    }
    close(socket_desc);

    return 0;
}

int * establishConnectionsFromExternalProcesses() {
    static int client_socket[numExternals];
    int server_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_size = sizeof(client_addr);

    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Socket creation failed");
        exit(1);
    }

    // Bind
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(1);
    }

    // Listen
    if (listen(server_socket, numExternals) < 0) {
        perror("Listen failed");
        exit(1);
    }

    printf("🌐 Server listening on port %d, waiting for %d clients...\n", PORT, numExternals);

    // Accept 4 clients
    for (int i = 0; i < numExternals; i++) {
        client_socket[i] = accept(server_socket, (struct sockaddr*)&client_addr, &client_size);
        if (client_socket[i] < 0) {
            perror("Accept failed");
            exit(1);
        }
        printf("✅ External process %d connected.\n", i);
    }

    close(server_socket); // close listening socket after accepting clients
    return client_socket;
}
