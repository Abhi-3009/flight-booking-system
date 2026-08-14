# Flight Booking System Makefile

CC = gcc
CFLAGS = -g -std=gnu11 -Wall
LDFLAGS = -lpthread

# Source files
SERVER_SRC = server.c auth.c customer.c employee.c manager.c admin.c utils.c session_manager.c
CLIENT_SRC = client.c

# Executables
SERVER = server
CLIENT = client

# Default target
all: $(SERVER) $(CLIENT)
	@echo "✓ Build complete!"

# Build server
$(SERVER): $(SERVER_SRC)
	$(CC) $(CFLAGS) $(SERVER_SRC) -o $(SERVER) $(LDFLAGS)

# Build client
$(CLIENT): $(CLIENT_SRC)
	$(CC) $(CFLAGS) $(CLIENT_SRC) -o $(CLIENT)

# Run server
run-server: $(SERVER)
	./$(SERVER)

# Run client
run-client: $(CLIENT)
	./$(CLIENT)

# Clean
clean:
	rm -f *.o $(SERVER) $(CLIENT)

# Setup data
setup:
	@mkdir -p data
	@echo "id,username,password,active" > data/customers.csv
	@echo "1,customer1,pass123,1" >> data/customers.csv
	@echo "2,customer2,pass123,1" >> data/customers.csv
	@echo "3,alice,alice123,1" >> data/customers.csv
	@echo "4,bob,bob123,1" >> data/customers.csv
	@echo "id,username,password" > data/employees.csv
	@echo "1,employee1,pass123" >> data/employees.csv
	@echo "id,username,password" > data/managers.csv
	@echo "1,manager1,pass123" >> data/managers.csv
	@echo "id,username,password" > data/admins.csv
	@echo "1,admin1,pass123" >> data/admins.csv
	@echo "flight_id,origin,destination,total_seats,available_seats" > data/flights.csv
	@echo "1,Delhi,Mumbai,150,150" >> data/flights.csv
	@echo "2,Mumbai,Bangalore,120,120" >> data/flights.csv
	@echo "booking_id,customer_id,flight_id,seats_booked,status,timestamp" > data/bookings.csv
	@echo "feedback_id,customer_id,message,reviewed" > data/feedback.csv
	@echo "✓ Data setup complete"

# Rebuild
rebuild: clean all

.PHONY: all clean run-server run-client setup rebuild
