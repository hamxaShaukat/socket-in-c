#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <ctype.h>
#define PORT 14000


void display_progress_bar(float progress) {
    int barWidth = 50;
    printf("[");
    int pos = barWidth * progress;
    for (int i = 0; i < barWidth; ++i) {
        if (i < pos) printf("=");
        else if (i == pos) printf(">");
        else printf(" ");
    }
    printf("] %d %%\r", (int)(progress * 100));
    fflush(stdout);
}


char* rle_encrypt(const char* input_string) {
    int length = strlen(input_string);
    if (length == 0) return ""; 

    char* output_string = malloc(2 * length + 1);
    if (!output_string) return NULL; 

    int j = 0; 
    int i = 0;

    while (i < length) {
        int count = 1; 
        while (i < length - 1 && input_string[i] == input_string[i + 1]) {
            count++;
            i++;
        }
        j += sprintf(&output_string[j], "%c%d", input_string[i], count);
        i++;
    }
    
    output_string[j] = '\0';
    char* resized_output = realloc(output_string, j + 1); 

    if (!resized_output) {
        free(output_string); 
        return NULL;
    }
    return resized_output;
}

char* rle_decode(const char* encoded_data) {
   
    char* decoded_data = malloc(1024 * 1024); 
    if (!decoded_data) return NULL;

    int i = 0, j = 0;

    while (encoded_data[i] != '\0') {
        char ch = encoded_data[i++];
        int count = 0;

        while (isdigit(encoded_data[i])) {
            count = count * 10 + (encoded_data[i] - '0');
            i++;
        }

        for (int k = 0; k < count; k++) {
            decoded_data[j++] = ch;
        }
    }

    decoded_data[j] = '\0'; // Null-terminate the decoded data
    char* resized_output = realloc(decoded_data, j + 1); 

    if (!resized_output) {
        // free(output_string); 
        return NULL;
    }
    return resized_output; // Resize to the actual size
}


void upload_file(int sock, const char *file_path) {
    char command[1024];
    snprintf(command, sizeof(command), "$UPLOAD$%s$", file_path);
    send(sock, command, strlen(command), 0);

    char response[1024] = {0};
    read(sock, response, sizeof(response));

    if (strcmp(response, "$SUCCESS$") == 0) {
        FILE *file = fopen(file_path, "rb");
        if (!file) {
            perror("File open failed");
            return;
        }

        struct stat st;
        stat(file_path, &st);
        long file_size = st.st_size;
        long total_sent = 0;

        char buffer[1024];
        
        // Read the file and encode it
        while (1) {
            int bytes_read = fread(buffer, 1, sizeof(buffer), file);
            if (bytes_read <= 0) break; // End of file

            // Encode the read data using RLE
            char* encoded_data = rle_encrypt(buffer);
            if (!encoded_data) {
                perror("RLE encoding failed");
                fclose(file);
                return;
            }

            printf("Encoded Data: %s\n", encoded_data);
            int encoded_length = strlen(encoded_data);
            // Send the encoded data
            send(sock, encoded_data, encoded_length, 0);
            total_sent += encoded_length;

            // Update progress bar
            display_progress_bar((float)total_sent / file_size);

            // Free the encoded data
            free(encoded_data);
        }

        fclose(file);
        shutdown(sock, SHUT_WR);  // Close the write end of the socket to signal completion
        read(sock, response, sizeof(response));
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

        long file_size;
        sscanf(response + 9, "%ld", &file_size);
        long total_received = 0;

        printf("Downloading file: %s\n", file_path);

        char buffer[1024];
        // Handle file content separately after the $SUCCESS$ message
        memmove(buffer, response + 9, bytes_read - 9);  // Move leftover data after $SUCCESS$
        fwrite(buffer, 1, bytes_read - 9, file);
         total_received += bytes_read - strlen("$SUCCESS$") - strlen(" ");

        // Read the rest of the file content from the socket
        while ((bytes_read = read(sock, buffer, sizeof(buffer))) > 0)
        {
            char* decoded_data = rle_decode(buffer);
            if (!decoded_data) {
                perror("RLE Decoding failed");
                break;
            }

            printf("Decoded Data: %s\n", decoded_data);
            int decoded_length = strlen(decoded_data); // Get length of decoded data
            fwrite(decoded_data, 1, decoded_length, file); // Write decoded data
            total_received += decoded_length;

            free(decoded_data);
            // Display progress bar
            display_progress_bar((float)total_received / file_size);
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
