#ifndef SEAT_MANAGER_H
#define SEAT_MANAGER_H

#include "common.h"
#include <time.h>

#define HOLD_TIMEOUT_SECONDS 120
#define MAX_ACTIVE_HOLDS 500

typedef struct {
    int flight_id;
    char seat_number[10];
    int customer_id;
    time_t hold_expires_at;
} SeatHold;

void init_seat_manager();
int init_flight_seats(int flight_id, int total_seats);
void display_seat_map(int sock, int flight_id);
int try_hold_seat(int flight_id, const char *seat_number, int customer_id, char *err_msg, int err_msg_len);
int confirm_seat_booking(int flight_id, const char *seat_number, int customer_id);
void release_seat_hold(int flight_id, const char *seat_number, int customer_id);
void release_customer_holds(int customer_id);
int cancel_booked_seat(int flight_id, const char *seat_number);

#endif
