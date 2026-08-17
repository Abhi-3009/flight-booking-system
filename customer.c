#include "customer.h"
#include "common.h"
#include "utils.h"
#include "session_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>

static void add_booking_record(int cid, int flight_id, int seats, const char *status) {
    int file_exists = (access(DATA_DIR "/bookings.csv", F_OK) == 0);
    FILE *fp = fopen(DATA_DIR "/bookings.csv", "a");
    if (!fp) return;
    if (fp && !file_exists) {
        fprintf(fp, "booking_id,customer_id,flight_id,seats_booked,status,timestamp\n");
    }

    time_t now = time(NULL);
    char timestamp[50];
    strftime(timestamp, 50, "%Y-%m-%d %H:%M:%S", localtime(&now));

    fprintf(fp, "%d,%d,%d,%d,%s,%s\n", get_next_id_csv(DATA_DIR "/bookings.csv"), cid, flight_id, seats, status, timestamp);
    fclose(fp);
}

static void view_available_flights(int sock) {
    FILE *fp = fopen(DATA_DIR "/flights.csv", "r");
    if (!fp) { send_message(sock, "No flights available.\n"); return; }

    char line[256], msg[BUFFER_SIZE];
    strcpy(msg, "\n--- Available Flights ---\nID | Origin -> Destination | Available Seats\n");
    send_message(sock, msg);

    fgets(line, sizeof(line), fp); // header
    while (fgets(line, sizeof(line), fp)) {
        int fid, tot, avail;
        char orig[50], dest[50];
        trim_string(line);
        if (sscanf(line, "%d,%49[^,],%49[^,],%d,%d", &fid, orig, dest, &tot, &avail) == 5) {
            snprintf(msg, BUFFER_SIZE, "%d | %s -> %s | %d seats\n", fid, orig, dest, avail);
            send_message(sock, msg);
        }
    }
    fclose(fp);
}

static void book_ticket(int sock, int cid) {
    char buffer[BUFFER_SIZE];
    send_message(sock, "Enter Flight ID to book: ");
    if (!read_input(sock, buffer, BUFFER_SIZE)) return;
    if (!is_numeric(buffer)) { send_message(sock, "Invalid Flight ID! Must be a number.\n"); return; }
    int target_fid = atoi(buffer);

    send_message(sock, "Enter number of seats to book: ");
    if (!read_input(sock, buffer, BUFFER_SIZE)) return;
    if (!is_numeric(buffer)) { send_message(sock, "Invalid seats! Must be a number.\n"); return; }
    int seats_to_book = atoi(buffer);
    if (seats_to_book <= 0) { send_message(sock, "Invalid seats.\n"); return; }

    char path[100];
    snprintf(path, 100, "%s/flights.csv", DATA_DIR);

    int fd = open(path, O_RDWR);
    if (fd == -1) { send_message(sock, "Error opening flights.\n"); return; }
    
    if (lock_file(fd, F_WRLCK) == -1) {
        close(fd); send_message(sock, "Server busy, try again.\n"); return;
    }

    FILE *fp = fdopen(dup(fd), "r");
    FILE *tmp = fopen(DATA_DIR "/flights.tmp", "w");

    if (!fp || !tmp) {
        if (fp) fclose(fp);
        if (tmp) fclose(tmp);
        unlock_file(fd); close(fd);
        send_message(sock, "Internal Error\n"); return;
    }

    char line[256];
    int found = 0;
    int success = 0;

    fgets(line, sizeof(line), fp);
    fprintf(tmp, "flight_id,origin,destination,total_seats,available_seats\n");

    while (fgets(line, sizeof(line), fp)) {
        int fid, tot, avail;
        char orig[50], dest[50];
        trim_string(line);
        if (sscanf(line, "%d,%49[^,],%49[^,],%d,%d", &fid, orig, dest, &tot, &avail) == 5) {
            if (fid == target_fid) {
                found = 1;
                if (avail >= seats_to_book) {
                    avail -= seats_to_book;
                    success = 1;
                }
            }
            fprintf(tmp, "%d,%s,%s,%d,%d\n", fid, orig, dest, tot, avail);
        }
    }

    fclose(fp); fclose(tmp);
    
    if (success) {
        rename(DATA_DIR "/flights.tmp", path);
    } else {
        remove(DATA_DIR "/flights.tmp");
    }
    
    unlock_file(fd); close(fd);

    if (!found) {
        send_message(sock, "Flight not found.\n");
    } else if (!success) {
        send_message(sock, "Not enough seats available!\n");
    } else {
        add_booking_record(cid, target_fid, seats_to_book, "CONFIRMED");
        send_message(sock, "Ticket booked successfully!\n");
    }
}

static void cancel_ticket(int sock, int cid) {
    char buffer[BUFFER_SIZE];
    send_message(sock, "Enter Booking ID to cancel: ");
    if (!read_input(sock, buffer, BUFFER_SIZE)) return;
    if (!is_numeric(buffer)) { send_message(sock, "Invalid Booking ID! Must be a number.\n"); return; }
    int target_bid = atoi(buffer);

    // 1. Mark booking as cancelled
    char path[100];
    snprintf(path, 100, "%s/bookings.csv", DATA_DIR);
    int fd = open(path, O_RDWR);
    if (fd == -1) { send_message(sock, "No bookings found.\n"); return; }
    if (lock_file(fd, F_WRLCK) == -1) { close(fd); return; }

    FILE *fp = fdopen(dup(fd), "r");
    FILE *tmp = fopen(DATA_DIR "/bookings.tmp", "w");
    
    char line[256];
    int flight_to_refund = -1;
    int seats_to_refund = 0;
    
    fgets(line, sizeof(line), fp);
    fprintf(tmp, "booking_id,customer_id,flight_id,seats_booked,status,timestamp\n");
    
    while (fgets(line, sizeof(line), fp)) {
        int bid, cus_id, fid, seats;
        char status[20], ts[50];
        trim_string(line);
        if (sscanf(line, "%d,%d,%d,%d,%19[^,],%49[^\n]", &bid, &cus_id, &fid, &seats, status, ts) == 6) {
            if (bid == target_bid && cus_id == cid && strcmp(status, "CONFIRMED") == 0) {
                strcpy(status, "CANCELLED");
                flight_to_refund = fid;
                seats_to_refund = seats;
            }
            fprintf(tmp, "%d,%d,%d,%d,%s,%s\n", bid, cus_id, fid, seats, status, ts);
        }
    }
    
    fclose(fp); fclose(tmp);
    rename(DATA_DIR "/bookings.tmp", path);
    unlock_file(fd); close(fd);
    
    if (flight_to_refund != -1) {
        // 2. Refund seats to flight
        char fpath[100];
        snprintf(fpath, 100, "%s/flights.csv", DATA_DIR);
        int ffd = open(fpath, O_RDWR);
        if (ffd != -1 && lock_file(ffd, F_WRLCK) != -1) {
            FILE *ffp = fdopen(dup(ffd), "r");
            FILE *ftmp = fopen(DATA_DIR "/flights.tmp", "w");
            fgets(line, sizeof(line), ffp);
            fprintf(ftmp, "flight_id,origin,destination,total_seats,available_seats\n");
            while (fgets(line, sizeof(line), ffp)) {
                int fid, tot, avail;
                char orig[50], dest[50];
                trim_string(line);
                if (sscanf(line, "%d,%49[^,],%49[^,],%d,%d", &fid, orig, dest, &tot, &avail) == 5) {
                    if (fid == flight_to_refund) {
                        avail += seats_to_refund;
                    }
                    fprintf(ftmp, "%d,%s,%s,%d,%d\n", fid, orig, dest, tot, avail);
                }
            }
            fclose(ffp); fclose(ftmp);
            rename(DATA_DIR "/flights.tmp", fpath);
            unlock_file(ffd); close(ffd);
        }
        send_message(sock, "Ticket cancelled successfully.\n");
    } else {
        send_message(sock, "Invalid booking ID or already cancelled.\n");
    }
}

static void view_my_bookings(int sock, int cid) {
    FILE *fp = fopen(DATA_DIR "/bookings.csv", "r");
    if (!fp) { send_message(sock, "No bookings.\n"); return; }
    
    char line[256], msg[BUFFER_SIZE];
    send_message(sock, "\n--- My Bookings ---\nID | Flight | Seats | Status | Date\n");
    fgets(line, sizeof(line), fp);
    
    while (fgets(line, sizeof(line), fp)) {
        int bid, cus_id, fid, seats;
        char status[20], ts[50];
        trim_string(line);
        if (sscanf(line, "%d,%d,%d,%d,%19[^,],%49[^\n]", &bid, &cus_id, &fid, &seats, status, ts) == 6) {
            if (cus_id == cid) {
                snprintf(msg, BUFFER_SIZE, "%d | F-%d | %d seats | %s | %s\n", bid, fid, seats, status, ts);
                send_message(sock, msg);
            }
        }
    }
    fclose(fp);
}

static void change_password(int sock, int cid) {
    char buffer[BUFFER_SIZE];
    send_message(sock, "Enter new password: ");
    if (!read_input(sock, buffer, BUFFER_SIZE)) return;
    
    char path[100];
    snprintf(path, 100, "%s/customers.csv", DATA_DIR);
    int fd = open(path, O_RDWR);
    if (fd == -1 || lock_file(fd, F_WRLCK) == -1) {
        if(fd != -1) close(fd);
        send_message(sock, "Error\n"); return;
    }
    
    FILE *fp = fdopen(dup(fd), "r");
    FILE *tmp = fopen(DATA_DIR "/customers.tmp", "w");
    
    char line[256];
    fgets(line, sizeof(line), fp);
    fprintf(tmp, "id,username,password,active\n");
    
    while(fgets(line, sizeof(line), fp)) {
        int id, active;
        char user[50], pass[50];
        trim_string(line);
        if(sscanf(line, "%d,%49[^,],%49[^,],%d", &id, user, pass, &active) == 4) {
            if(id == cid) {
                strcpy(pass, buffer);
            }
            fprintf(tmp, "%d,%s,%s,%d\n", id, user, pass, active);
        }
    }
    fclose(fp); fclose(tmp);
    rename(DATA_DIR "/customers.tmp", path);
    unlock_file(fd); close(fd);
    send_message(sock, "Password updated.\n");
}

static void add_feedback(int sock, int cid) {
    char buffer[BUFFER_SIZE];
    send_message(sock, "Enter your feedback: ");
    if(read_input(sock, buffer, BUFFER_SIZE)) {
        FILE *fp = fopen(DATA_DIR "/feedback.txt", "a");
        if(fp) {
            fprintf(fp, "Passenger %d: %s\n", cid, buffer);
            fclose(fp);
            send_message(sock, "Feedback submitted.\n");
        }
    }
}

void handle_customer(int sock, int cid) {
    char buffer[BUFFER_SIZE];

    while (1) {
        send_message(sock,
            "\n=== Passenger Menu ===\n"
            "1. View Available Flights\n"
            "2. Book Ticket\n"
            "3. Cancel Ticket\n"
            "4. View My Bookings\n"
            "5. Change Password\n"
            "6. Add Feedback\n"
            "7. Logout\n"
            "Choice: ");

        if (!read_input(sock, buffer, BUFFER_SIZE)) break;
        if (!is_numeric(buffer)) { send_message(sock, "Invalid choice! Please enter a number.\n"); continue; }
        int choice = atoi(buffer);

        switch (choice) {
            case 1: view_available_flights(sock); break;
            case 2: book_ticket(sock, cid); break;
            case 3: cancel_ticket(sock, cid); break;
            case 4: view_my_bookings(sock, cid); break;
            case 5: change_password(sock, cid); break;
            case 6: add_feedback(sock, cid); break;
            case 7: return;
            default: send_message(sock, "Invalid choice\n");
        }
    }
}
