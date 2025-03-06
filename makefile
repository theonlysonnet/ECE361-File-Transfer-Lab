# Compiler to use
CC = gcc

# Directories for executables
DELIVER_DIR = deliverLab2
SERVER_DIR = serverLab2

# Targets
all: $(DELIVER_DIR)/deliver $(SERVER_DIR)/server

# Build deliver
$(DELIVER_DIR)/deliver: deliver.c | $(DELIVER_DIR)
	$(CC) -o $@ deliver.c

# Build server
$(SERVER_DIR)/server: server.c | $(SERVER_DIR)
	$(CC) -o $@ server.c

# Create directories if they don't exist
$(DELIVER_DIR):
	mkdir -p $@

$(SERVER_DIR):
	mkdir -p $@

# Clean build artifacts
clean:
	rm -f $(DELIVER_DIR)/deliver $(SERVER_DIR)/server