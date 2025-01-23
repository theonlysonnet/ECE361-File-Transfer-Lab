# Compiler to use
CC = gcc

# Targets
all: deliver server

# Build deliver
deliver: deliver.c
	$(CC) -o deliver deliver.c

# Build server
server: server.c
	$(CC) -o server server.c

# Clean build artifacts
clean:
	rm -f deliver server
