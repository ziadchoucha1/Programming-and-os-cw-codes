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

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr);

    if (connect(sock, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    int client_id;
    int access_level;
    char message[12]; // 12 chars + 4 bytes ID = 16 bytes total

    printf("Enter client ID: ");
    scanf("%d", &client_id);

    printf("Enter access level (0 = Admin, 1 = User, 2 = Guest): ");
    scanf("%d", &access_level);

    printf("Enter command: ");
    scanf("%s", message);

    unsigned char plain[16] = {0};

    // Pack: [ID (4 bytes)] + [access level (4 bytes)] + [command]
    memcpy(plain, &client_id, sizeof(int));
    memcpy(plain + 4, &access_level, sizeof(int));
    memcpy(plain + 8, message, strlen(message));

    encrypt(plain, encrypted_message);

    send(sock, encrypted_message, 16, 0);

    // Receive and decrypt the response
    read(sock, buffer, 16);
    decrypt(buffer, decrypted_response);

    printf("Server response: %s\n", decrypted_response + 4);  // Skip access level and ID

    close(sock);
    return 0;
}
