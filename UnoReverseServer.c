#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include <curl/curl.h>

#define PORT            22
#define BUFFER_SIZE     1024
#define LOG_FILE        "log.txt"
#define GEO_DATA_SIZE   2048
#define JUNK_SIZE       1024
#define MB10_COUNT      10240  // 10240 * 1024B = 10MB

typedef struct {
    char buffer[GEO_DATA_SIZE];
    size_t current_size;
} GeoBuffer;

size_t write_callback(void *ptr, size_t size, size_t nmemb, void *userdata) {
    GeoBuffer *geo_buf = (GeoBuffer *)userdata;
    size_t data_size = size * nmemb;
    size_t remaining = GEO_DATA_SIZE - geo_buf->current_size - 1;

    if (remaining == 0) return 0;
    size_t copy_size = data_size < remaining ? data_size : remaining;

    memcpy(geo_buf->buffer + geo_buf->current_size, ptr, copy_size);
    geo_buf->current_size += copy_size;
    geo_buf->buffer[geo_buf->current_size] = '\0';

    return copy_size;
}

void log_to_file(const char *ip, const char *message, const char *geo, int bytes_sent) {
    FILE *log_file = fopen(LOG_FILE, "a");
    if (!log_file) {
        perror("Failed to open log file");
        return;
    }

    time_t now = time(NULL);
    struct tm tm_info;
    localtime_r(&now, &tm_info);
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &tm_info);

    fprintf(log_file, "[%s] IP: %s\n", timestamp, ip);
    fprintf(log_file, "Received data: %s\n", message);
    fprintf(log_file, "Geolocation: %s\n", geo);
    fprintf(log_file, "Reverse attack executed (%d bytes sent)\n\n", bytes_sent);

    fclose(log_file);
}

void fetch_geolocation(const char *ip, char *result, size_t result_size) {
    CURL *curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "CURL initialization failed\n");
        return;
    }

    char url[256];
    snprintf(url, sizeof(url), "http://ip-api.com/json/%s", ip);

    GeoBuffer geo_buf = {0};
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &geo_buf);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        fprintf(stderr, "CURL request failed: %s\n", curl_easy_strerror(res));
    }

    strncpy(result, geo_buf.buffer, result_size - 1);
    result[result_size - 1] = '\0';
    curl_easy_cleanup(curl);
}

int main() {
    int sockfd, client_sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addrlen = sizeof(client_addr);
    char buffer[BUFFER_SIZE];
    char client_ip[INET_ADDRSTRLEN];
    char geo_data[GEO_DATA_SIZE];
    char junk[JUNK_SIZE];

    curl_global_init(CURL_GLOBAL_DEFAULT);

    // Create TCP socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Configure server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind socket to port
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed (requires root privileges for port 22)");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Start listening for connections
    if (listen(sockfd, 3) < 0) {
        perror("Listen failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("UnoReverse Server listening on port %d...\n", PORT);

    while (1) {
        // Accept incoming connection
        if ((client_sockfd = accept(sockfd, (struct sockaddr *)&client_addr, &addrlen)) < 0) {
            perror("Connection acceptance failed");
            continue;
        }

        // Get client IP address
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
        printf("Connection from %s\n", client_ip);

        // Receive client data
        memset(buffer, 0, BUFFER_SIZE);
        ssize_t bytes_read = recv(client_sockfd, buffer, BUFFER_SIZE - 1, 0);
        
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            memset(geo_data, 0, GEO_DATA_SIZE);

            // Get geolocation information
            fetch_geolocation(client_ip, geo_data, GEO_DATA_SIZE);

            // Execute reverse attack (send 10MB)
            int total_sent = 0;
            for (int i = 0; i < MB10_COUNT; i++) {
                ssize_t bytes_sent = send(client_sockfd, junk, JUNK_SIZE, 0);
                if (bytes_sent < 0) {
                    perror("Data transmission failed");
                    break;
                }
                total_sent += bytes_sent;
            }

            // Log connection details
            log_to_file(client_ip, buffer, geo_data, total_sent);
        }

        close(client_sockfd);
    }

    close(sockfd);
    curl_global_cleanup();
    return EXIT_SUCCESS;
}