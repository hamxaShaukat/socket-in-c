# Compiler
CC = gcc

# Compiler Flags
CFLAGS = -Wall -pthread

# Source Files
SERVER_SRC = server.c
CLIENT_SRC = client.c

# Output Binaries
SERVER_BIN = server
CLIENT_BIN = client

# Targets

# Build server
$(SERVER_BIN): $(SERVER_SRC)
	$(CC) $(CFLAGS) -o $(SERVER_BIN) $(SERVER_SRC)

# Build client
$(CLIENT_BIN): $(CLIENT_SRC)
	$(CC) $(CFLAGS) -o $(CLIENT_BIN) $(CLIENT_SRC)

# Build both server and client
all: $(SERVER_BIN) $(CLIENT_BIN)

# Clean up binary files
clean:
	rm -f $(SERVER_BIN) $(CLIENT_BIN)
