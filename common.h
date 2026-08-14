#ifndef COMMON_H
#define COMMON_H

#define PORT 9091
#define BUFFER_SIZE 1024
#define DATA_DIR "data"
#define MAX_SESSIONS 100
#define SHM_KEY_SESSIONS 9999

typedef struct {
    int id;
    char username[50];
    char password[50];
    int active;
} Customer;

typedef struct {
    int id;
    char username[50];
    char password[50];
} Employee;

typedef struct {
    int id;
    char username[50];
    char password[50];
} Manager;

typedef struct {
    int id;
    char username[50];
    char password[50];
} Admin;

typedef struct {
    int booking_id;
    int customer_id;
    int flight_id;
    int seats_booked;
    char status[20];
    char timestamp[50];
} BookingRecord;

typedef struct {
    int flight_id;
    char origin[50];
    char destination[50];
    int total_seats;
    int available_seats;
} Flight;

typedef struct {
    int user_id;
    int role;
    int active;
} Session;

typedef struct {
    Session sessions[MAX_SESSIONS];
} SessionManager;

#define ROLE_CUSTOMER 1
#define ROLE_EMPLOYEE 2
#define ROLE_MANAGER 3
#define ROLE_ADMIN 4

#endif
