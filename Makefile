# Flight Booking System Makefile

CC = gcc
CFLAGS = -g -std=gnu11 -Wall
LDFLAGS = -lpthread

# Source files
SERVER_SRC = server.c auth.c customer.c employee.c manager.c admin.c utils.c session_manager.c seat_manager.c bcrypt.c
CLIENT_SRC = client.c

# Executables
SERVER = server
CLIENT = client
INIT_DB = init_db

# Default target
all: $(SERVER) $(CLIENT)
	@echo "✓ Build complete!"

# Build server
$(SERVER): $(SERVER_SRC)
	$(CC) $(CFLAGS) $(SERVER_SRC) -o $(SERVER) $(LDFLAGS)

# Build client
$(CLIENT): $(CLIENT_SRC)
	$(CC) $(CFLAGS) $(CLIENT_SRC) -o $(CLIENT)

# Build init_db tool
$(INIT_DB): init_db.c bcrypt.c
	$(CC) $(CFLAGS) init_db.c bcrypt.c -o $(INIT_DB)

# Run server
run-server: $(SERVER)
	./$(SERVER)

# Run client
run-client: $(CLIENT)
	./$(CLIENT)

# Clean
clean:
	rm -f *.o $(SERVER) $(CLIENT) $(INIT_DB) test_bcrypt

# Setup data
setup: $(INIT_DB)
	@mkdir -p data
	@rm -f data/loans.csv data/transactions.csv data/*.tmp
	@./$(INIT_DB) > /dev/null
	@echo "flight_id,origin,destination,total_seats,available_seats" > data/flights.csv
	@echo "1,Delhi,Mumbai,20,20" >> data/flights.csv
	@echo "2,Mumbai,Bangalore,20,20" >> data/flights.csv
	@echo "booking_id,customer_id,flight_id,seat_number,seats_booked,status,timestamp" > data/bookings.csv
	@echo "flight_id,seat_number,status" > data/seats.csv
	@touch data/feedback.txt
	@echo "✓ Data setup complete (bcrypt password hashes initialized)"

# Test
test:
	python3 test_system.py
	python3 test_all_roles.py

# Rebuild
rebuild: clean all

.PHONY: all clean run-server run-client setup rebuild test
