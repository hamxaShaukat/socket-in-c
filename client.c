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
void view_files(int sock) {
    char command[1024];
    snprintf(command, sizeof(command), "$VIEW$");
    send(sock, command, strlen(command), 0);

    char response[1024] = {0};
    read(sock, response, 1024);

    if (strcmp(response, "$FAILURE$NO_CLIENT_DATA$") == 0) {
        printf("No files found for the client.\n");
    } else {
        printf("Files:\n%s\n", response);
    }
}

void download_file(int sock, const char *file_name) {
    char command[1024];
    snprintf(command, sizeof(command), "$DOWNLOAD$%s$", file_name);
    send(sock, command, strlen(command), 0);

    // Receive initial response from the server
    char response[1024] = {0};
    int bytes_read = read(sock, response, sizeof(response) - 1);
    response[bytes_read] = '\0';  // Null-terminate the response

    if (strcmp(response, "$FAILURE$FILE_NOT_FOUND$") == 0) {
        printf("Server Response: %s\n", response);
        return;
    } else if (strncmp(response, "$SUCCESS$", 9) == 0) {
        char file_path[1024];
        snprintf(file_path, sizeof(file_path), "/home/%s/Downloads/%s", getenv("USER"), file_name);

        FILE *file = fopen(file_path, "wb");
        if (!file) {
            perror("File open failed");
            return;
        }

        printf("Downloading file: %s\n", file_path);

        char buffer[1024];
        // Handle file content separately after the $SUCCESS$ message
        memmove(buffer, response + 9, bytes_read - 9);  // Move leftover data after $SUCCESS$
        fwrite(buffer, 1, bytes_read - 9, file);

        // Read the rest of the file content from the socket
        while ((bytes_read = read(sock, buffer, sizeof(buffer))) > 0) {
            fwrite(buffer, 1, bytes_read, file);
        }

        fclose(file);
        printf("File downloaded successfully to %s\n", file_path);
    } else {
        printf("Unexpected server response: %s\n", response);
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

    char operation[10];
    printf("Enter operation (upload/download/view): ");
    fgets(operation, sizeof(operation), stdin);
    operation[strcspn(operation, "\n")] = 0;  // Remove the newline character

    if (strcmp(operation, "upload") == 0) {
        char file_path[1024];
        printf("Enter the path of the file to upload: ");
        fgets(file_path, sizeof(file_path), stdin);
        file_path[strcspn(file_path, "\n")] = 0;  // Remove the newline character
        upload_file(sock, file_path);
    } else if (strcmp(operation, "download") == 0) {
        char file_name[1024];
        printf("Enter the name of the file to download: ");
        fgets(file_name, sizeof(file_name), stdin);
        file_name[strcspn(file_name, "\n")] = 0;  // Remove the newline character
        download_file(sock, file_name);
    } else if (strcmp(operation, "view") == 0) {
    view_files(sock);
}


    close(sock);
    return 0;
}
