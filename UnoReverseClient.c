#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SERVER_IP    "127.0.0.1"
#define SERVER_PORT  22
#define BUFFER_SIZE  1024

int main() {
    int sockfd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_received;
    ssize_t bytes_sent;
    int total_bytes_received = 0;

    // Create TCP socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Configure server address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("Invalid server address");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Establish connection with server
   if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Successfully connected to server at %s:%d\n", SERVER_IP, SERVER_PORT);

    // Send fake credentials to server
    const char *fake_credentials = "admin:admin123";
    bytes_sent = send(sockfd, fake_credentials, strlen(fake_credentials), 0);
    if (bytes_sent < 0) {
        perror("Failed to send credentials");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    printf("Sent fake credentials: %s\n", fake_credentials);

    // Receive server response (simulate 10MB data transfer)
    while ((bytes_received = recv(sockfd, buffer, BUFFER_SIZE, 0)) > 0) {
        total_bytes_received += bytes_received;
    }

    if (bytes_received < 0) {
        perror("Data reception error");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Received total reverse data: %d bytes\n", total_bytes_received);
    close(sockfd);
    return EXIT_SUCCESS;
}