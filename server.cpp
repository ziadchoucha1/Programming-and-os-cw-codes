#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <openssl/aes.h>
#include <pthread.h>

#define PORT 8080
#define BUFFER_SIZE 1024

// Pre-shared AES key (16 bytes = AES-128)
unsigned char AES_KEY_STRING[16] = "mysecretkey1234";

void decrypt(unsigned char *ciphertext, unsigned char *plaintext) {
    AES_KEY dec_key;
    AES_set_decrypt_key(AES_KEY_STRING, 128, &dec_key);

    AES_decrypt(ciphertext, plaintext, &dec_key);
}

void encrypt(unsigned char *plaintext, unsigned char *ciphertext) {
    AES_KEY enc_key;
    AES_set_encrypt_key(AES_KEY_STRING, 128, &enc_key);

    AES_encrypt(plaintext, ciphertext, &enc_key);
}
void* handleClient(void* arg) {
    int client_socket = *(int*)arg;
    free(arg);  // prevent memory leak

    unsigned char buffer[16] = {0};
    unsigned char decrypted[16] = {0};
    unsigned char encrypted_response[16] = {0};

    char response[] = "Hello Secure serv";

    // Receive encrypted message
    int bytesRead = read(client_socket, buffer, 16);
    if (bytesRead <= 0) {
        close(client_socket);
        return NULL;
    }

    // Decrypt
    decrypt(buffer, decrypted);
    decrypted[15] = '\0';

    printf("Client message: %s\n", decrypted);

    // Encrypt response
    encrypt((unsigned char*)response, encrypted_response);

    // Send response
    send(client_socket, encrypted_response, 16, 0);

    close(client_socket);
    return NULL;
}
int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    unsigned char buffer[BUFFER_SIZE] = {0};
    unsigned char decrypted[BUFFER_SIZE] = {0};
    unsigned char encrypted_response[BUFFER_SIZE] = {0};

    char response[] = "Hello from secure server";

    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    // Define server address
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind socket
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen
    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Secure Server listening on port %d...\n", PORT);

    // Accept connection
    while (1) {
    int* new_socket = (int*)malloc(sizeof(int));

    *new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);

    if (*new_socket < 0) {
        perror("Accept failed");
        free(new_socket);
        continue;
    }

    pthread_t thread_id;

    if (pthread_create(&thread_id, NULL, handleClient, (void*)new_socket) != 0) {
        perror("Thread creation failed");
        close(*new_socket);
        free(new_socket);
    }

    pthread_detach(thread_id);  // auto cleanup
}
    // Receive encrypted message
    read(new_socket, buffer, BUFFER_SIZE);

    // Decrypt message
    decrypt(buffer, decrypted);

    printf("Encrypted message received.\n");
    printf("Decrypted client message: %s\n", decrypted);

    // Encrypt server response
    encrypt((unsigned char*)response, encrypted_response);

    // Send encrypted response
    send(new_socket, encrypted_response, BUFFER_SIZE, 0);

    // Close sockets
    close(new_socket);
    close(server_fd);

    return 0;
}
