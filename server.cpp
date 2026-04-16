#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <openssl/aes.h>
#include <pthread.h>

#define PORT 8080
#define BUFFER_SIZE 1024  // Increased buffer size for command execution results

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

// Function to check if the user has permission to run the requested command
int has_permission(int access_level, const char *command) {
    printf("Access Level: %d, Command Requested: %s\n", access_level, command);  // Debug output

    // Admin (0) can run everything
    if (access_level == 0) {
        return 1;  // Admin can execute all commands
    }

    // User (1) can run basic commands like 'ls', 'pwd'
    if (access_level == 1) {
        if (strcmp(command, "ls") == 0 || strcmp(command, "pwd") == 0 || strcmp(command, "whoami") == 0) {
            return 1;  // User can run 'ls', 'pwd', 'whoami'
        }
        return 0;  // No permission for other commands
    }

    // Guest (2) can only run 'ls' and 'pwd'
    if (access_level == 2) {
        if (strcmp(command, "ls") == 0 || strcmp(command, "pwd") == 0) {
            return 1;  // Guest can only run 'ls' and 'pwd'
        }
        return 0;  // No permission for other commands
    }

    return 0;  // Default no permission for unhandled cases
}

void* handleClient(void* arg) {
    int client_socket = *(int*)arg;
    free(arg);

    unsigned char buffer[BUFFER_SIZE] = {0};
    unsigned char decrypted[BUFFER_SIZE] = {0};
    unsigned char encrypted_response[BUFFER_SIZE] = {0};

    // Read encrypted command from client
    int bytesRead = read(client_socket, buffer, BUFFER_SIZE);
    if (bytesRead <= 0) {
        close(client_socket);
        return NULL;
    }

    // Decrypt the data
    decrypt(buffer, decrypted);

    // Extract client ID, access level, and command
    int client_id, access_level;
    memcpy(&client_id, decrypted, sizeof(int));  // First 4 bytes for ID
    memcpy(&access_level, decrypted + 4, sizeof(int));  // Next 4 bytes for access level

    char client_command[BUFFER_SIZE] = {0};
    memcpy(client_command, decrypted + 8, bytesRead - sizeof(int) - sizeof(int));  // Extract command

    printf("Client %d with access level %d requested to execute: %s\n", client_id, access_level, client_command);

    // Check if the client has permission to execute the command
    if (!has_permission(access_level, client_command)) {
        snprintf((char*)encrypted_response, sizeof(encrypted_response), "Permission Denied: Access to command '%s' is restricted for your access level.", client_command);
        encrypt(encrypted_response, encrypted_response);
        send(client_socket, encrypted_response, BUFFER_SIZE, 0);
        close(client_socket);
        return NULL;
    }

    // Execute the command using popen
    FILE *fp;
    char result[BUFFER_SIZE * 2] = {0};  // Increased buffer size for result

    // Attempt to open the command using popen
    fp = popen(client_command, "r");
    if (fp == NULL) {
        snprintf(result, sizeof(result), "Error executing command.");
        printf("Error executing command: %s\n", client_command);
    } else {
        while (fgets(result, sizeof(result) - 1, fp) != NULL) {
            // Keep appending to the result buffer
            printf("Command Output: %s\n", result);  // Add this line for more debug info
        }
        fclose(fp);
    }

    // Encrypt the response (command result)
    encrypt((unsigned char*)result, encrypted_response);

    // Send the encrypted response to the client
    send(client_socket, encrypted_response, BUFFER_SIZE, 0);

    close(client_socket);
    return NULL;
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // Create server socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    // Define server address
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind the socket
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Secure Server listening on port %d...\n", PORT);

    // Accept and handle client connections
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

        pthread_detach(thread_id);  // Auto cleanup after the thread finishes
    }

    close(server_fd);
    return 0;
}
