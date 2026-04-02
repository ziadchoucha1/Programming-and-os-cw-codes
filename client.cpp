#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <openssl/aes.h>

#define PORT 8080
#define BUFFER_SIZE 1024

// Pre-shared AES key (16 bytes = AES-128)
unsigned char AES_KEY_STRING[16] = "mysecretkey1234";

void encrypt(unsigned char *plaintext, unsigned char *ciphertext) {
    AES_KEY enc_key;
    AES_set_encrypt_key(AES_KEY_STRING, 128, &enc_key);
    AES_encrypt(plaintext, ciphertext, &enc_key);
}

void decrypt(unsigned char *ciphertext, unsigned char *plaintext) {
    AES_KEY dec_key;
    AES_set_decrypt_key(AES_KEY_STRING, 128, &dec_key);
    AES_decrypt(ciphertext, plaintext, &dec_key);
}

int main() {
    int sock;
    struct sockaddr_in server_address;

    unsigned char buffer[BUFFER_SIZE] = {0};
    unsigned char encrypted_message[BUFFER_SIZE] = {0};
    unsigned char decrypted_response[BUFFER_SIZE] = {0};

    // Create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    // Define server address
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr);

    // Connect to server
    if (connect(sock, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    // Original message
    unsigned char message[16] = "HelloClient1234";

    // Encrypt the message
    encrypt(message, encrypted_message);

    // Send encrypted message
    send(sock, encrypted_message, 16, 0);
    printf("Encrypted message sent to server.\n");

    // Receive encrypted response
    read(sock, buffer, 16);

    // Decrypt server response
    decrypt(buffer, decrypted_response);

    //Ensure string ends
    decrypted_response[15] = '\0';

    printf("Server: %s\n", decrypted_response);

    // Close socket
    close(sock);

    return 0;
}
