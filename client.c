#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 14000

void upload_file(int sock, const char *file_path) {
    char command[1024];
    snprintf(command, sizeof(command), "$UPLOAD$%s$", file_path);
    send(sock, command, strlen(command), 0);

    char response[1024] = {0};
    read(sock, response, 1024);

    if (strcmp(response, "$SUCCESS$") == 0) {
        FILE *file = fopen(file_path, "rb");
        if (!file) {
            perror("File open failed");
            return;
        }

        char buffer[1024];
        int bytes_read;
        while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
            send(sock, buffer, bytes_read, 0);
        }

        fclose(file);
        shutdown(sock, SHUT_WR);  // Close the write end of the socket to signal completion
        read(sock, response, 1024);
        printf("Server Response: %s\n", response);
    } else {
        printf("Server Response: %s\n", response);
    }
}

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Socket creation error\n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("Invalid address/Address not supported\n");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("Connection failed\n");
        return -1;
    }

    char file_path[1024];
    printf("Enter the path of the file to upload: ");
    fgets(file_path, sizeof(file_path), stdin);
    file_path[strcspn(file_path, "\n")] = 0;  // Remove the newline character

    // Upload the file specified by the user
    upload_file(sock, file_path);

    close(sock);

    return 0;
}
