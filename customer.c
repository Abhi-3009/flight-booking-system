#include "customer.h"
#include "common.h"
#include "utils.h"
#include "session_manager.h"
#include "seat_manager.h"
#include "bcrypt.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>

static void add_booking_record(int cid, int flight_id, const char *seat_no, int seats, const char *status) {
    char path[100];
    snprintf(path, sizeof(path), "%s/bookings.csv", DATA_DIR);

    int file_exists = (access(path, F_OK) == 0);
    int next_id = get_next_id_csv(path);

    FILE *fp = fopen(path, "a");
    if (!fp) return;

    if (!file_exists) {
        fprintf(fp, "booking_id,customer_id,flight_id,seat_number,seats_booked,status,timestamp\n");
    }

    time_t now = time(NULL);
    char timestamp[50];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));

    fprintf(fp, "%d,%d,%d,%s,%d,%s,%s\n", next_id, cid, flight_id, seat_no, seats, status, timestamp);
    fclose(fp);
}

static void view_available_flights(int sock) {
    char path[100];
    snprintf(path, sizeof(path), "%s/flights.csv", DATA_DIR);
    FILE *fp = fopen(path, "r");
    if (!fp) {
        send_message(sock, "No flights available.\n");
        return;
    }

    char line[256], msg[BUFFER_SIZE];
    strcpy(msg, "\n--- Available Flights ---\nID | Origin -> Destination | Available Seats\n");
    send_message(sock, msg);

    fgets(line, sizeof(line), fp); // Header
    while (fgets(line, sizeof(line), fp)) {
        int fid, tot, avail;
        char orig[50], dest[50];
        trim_string(line);
        if (sscanf(line, "%d,%49[^,],%49[^,],%d,%d", &fid, orig, dest, &tot, &avail) == 5) {
            snprintf(msg, BUFFER_SIZE, "%d  | %s -> %s | %d seats\n", fid, orig, dest, avail);
            send_message(sock, msg);
        }
    }
    fclose(fp);
}

static void book_ticket(int sock, int cid) {
    char buffer[BUFFER_SIZE];

    view_available_flights(sock);

    send_message(sock, "\nEnter Flight ID to book (or 0 to cancel): ");
    if (!read_input(sock, buffer, BUFFER_SIZE)) return;
    if (!is_numeric(buffer)) {
        send_message(sock, "Invalid Flight ID! Must be a number.\n");
        return;
    }
    int target_fid = atoi(buffer);
    if (target_fid <= 0) return;

    // Verify flight exists
    char path[100];
    snprintf(path, sizeof(path), "%s/flights.csv", DATA_DIR);
    FILE *fp = fopen(path, "r");
    if (!fp) {
        send_message(sock, "Error opening flights database.\n");
        return;
    }

    char line[256];
    int flight_found = 0;
    int avail_seats = 0;
    fgets(line, sizeof(line), fp);
    while (fgets(line, sizeof(line), fp)) {
        int fid, tot, avail;
        char orig[50], dest[50];
        trim_string(line);
        if (sscanf(line, "%d,%49[^,],%49[^,],%d,%d", &fid, orig, dest, &tot, &avail) == 5) {
            if (fid == target_fid) {
                flight_found = 1;
                avail_seats = avail;
                break;
            }
        }
    }
    fclose(fp);

    if (!flight_found) {
        send_message(sock, "Flight ID not found.\n");
        return;
    }

    if (avail_seats <= 0) {
        send_message(sock, "Sorry, this flight is fully booked!\n");
        return;
    }

    // Display visual seat map
    display_seat_map(sock, target_fid);

    send_message(sock, "Enter Seat Number to reserve (e.g. 1A, 2B, or '0' to exit): ");
    if (!read_input(sock, buffer, BUFFER_SIZE)) return;
    trim_string(buffer);
    if (strcmp(buffer, "0") == 0 || strlen(buffer) == 0) {
        send_message(sock, "Booking cancelled.\n");
        return;
    }

    char selected_seat[10];
    strncpy(selected_seat, buffer, sizeof(selected_seat) - 1);
    selected_seat[sizeof(selected_seat) - 1] = '\0';

    char err_msg[BUFFER_SIZE];
    if (!try_hold_seat(target_fid, selected_seat, cid, err_msg, sizeof(err_msg))) {
        send_message(sock, "\n✗ ");
        send_message(sock, err_msg);
        return;
    }

    snprintf(err_msg, sizeof(err_msg),
        "\n✓ Seat %s reserved for you! (Hold active for %d seconds)\n"
        "Confirm booking? (Y/N): ",
        selected_seat, HOLD_TIMEOUT_SECONDS);
    send_message(sock, err_msg);

    if (!read_input(sock, buffer, BUFFER_SIZE)) {
        release_seat_hold(target_fid, selected_seat, cid);
        return;
    }

    trim_string(buffer);
    if (strcasecmp(buffer, "Y") != 0 && strcasecmp(buffer, "YES") != 0) {
        release_seat_hold(target_fid, selected_seat, cid);
        send_message(sock, "Reservation cancelled. Seat released.\n");
        return;
    }

    // Confirm booking
    if (!confirm_seat_booking(target_fid, selected_seat, cid)) {
        send_message(sock, "\n✗ Hold expired or could not complete booking. Please try again.\n");
        return;
    }

    // Decrement available seats in flights.csv
    int fd = open(path, O_RDWR);
    if (fd != -1) {
        lock_file(fd, F_WRLCK);
        FILE *ffp = fdopen(dup(fd), "r");
        FILE *ftmp = fopen(DATA_DIR "/flights.tmp", "w");
        if (ffp && ftmp) {
            fgets(line, sizeof(line), ffp);
            fprintf(ftmp, "flight_id,origin,destination,total_seats,available_seats\n");
            while (fgets(line, sizeof(line), ffp)) {
                int fid, tot, avail;
                char orig[50], dest[50];
                trim_string(line);
                if (sscanf(line, "%d,%49[^,],%49[^,],%d,%d", &fid, orig, dest, &tot, &avail) == 5) {
                    if (fid == target_fid && avail > 0) {
                        avail -= 1;
                    }
                    fprintf(ftmp, "%d,%s,%s,%d,%d\n", fid, orig, dest, tot, avail);
                }
            }
            fclose(ffp);
            fclose(ftmp);
            rename(DATA_DIR "/flights.tmp", path);
        } else {
            if (ffp) fclose(ffp);
            if (ftmp) fclose(ftmp);
        }
        unlock_file(fd);
        close(fd);
    }

    add_booking_record(cid, target_fid, selected_seat, 1, "CONFIRMED");

    snprintf(err_msg, sizeof(err_msg),
        "\n🎉 Congratulations! Ticket booked successfully for Flight %d, Seat %s!\n",
        target_fid, selected_seat);
    send_message(sock, err_msg);
}

static void cancel_ticket(int sock, int cid) {
    char buffer[BUFFER_SIZE];
    send_message(sock, "Enter Booking ID to cancel: ");
    if (!read_input(sock, buffer, BUFFER_SIZE)) return;
    if (!is_numeric(buffer)) {
        send_message(sock, "Invalid Booking ID! Must be a number.\n");
        return;
    }
    int target_bid = atoi(buffer);

    char path[100];
    snprintf(path, sizeof(path), "%s/bookings.csv", DATA_DIR);
    int fd = open(path, O_RDWR);
    if (fd == -1) {
        send_message(sock, "No bookings found.\n");
        return;
    }
    if (lock_file(fd, F_WRLCK) == -1) {
        close(fd);
        send_message(sock, "Server busy. Try again.\n");
        return;
    }

    FILE *fp = fdopen(dup(fd), "r");
    FILE *tmp = fopen(DATA_DIR "/bookings.tmp", "w");
    if (!fp || !tmp) {
        if (fp) fclose(fp);
        if (tmp) fclose(tmp);
        unlock_file(fd);
        close(fd);
        send_message(sock, "Internal Error.\n");
        return;
    }

    char line[256];
    int flight_to_refund = -1;
    char seat_to_refund[10] = "";
    int seats_to_refund = 0;

    fgets(line, sizeof(line), fp);
    fprintf(tmp, "booking_id,customer_id,flight_id,seat_number,seats_booked,status,timestamp\n");

    while (fgets(line, sizeof(line), fp)) {
        int bid, cus_id, fid, seats;
        char seat_no[10] = "-", status[20], ts[50];
        trim_string(line);

        // Try 7-field format first, fallback to 6-field format for backwards compatibility
        if (sscanf(line, "%d,%d,%d,%9[^,],%d,%19[^,],%49[^\n]", &bid, &cus_id, &fid, seat_no, &seats, status, ts) == 7) {
            if (bid == target_bid && cus_id == cid && strcmp(status, "CONFIRMED") == 0) {
                strcpy(status, "CANCELLED");
                flight_to_refund = fid;
                strncpy(seat_to_refund, seat_no, sizeof(seat_to_refund) - 1);
                seats_to_refund = seats;
            }
            fprintf(tmp, "%d,%d,%d,%s,%d,%s,%s\n", bid, cus_id, fid, seat_no, seats, status, ts);
        } else if (sscanf(line, "%d,%d,%d,%d,%19[^,],%49[^\n]", &bid, &cus_id, &fid, &seats, status, ts) == 6) {
            if (bid == target_bid && cus_id == cid && strcmp(status, "CONFIRMED") == 0) {
                strcpy(status, "CANCELLED");
                flight_to_refund = fid;
                seats_to_refund = seats;
            }
            fprintf(tmp, "%d,%d,%d,%s,%d,%s,%s\n", bid, cus_id, fid, "-", seats, status, ts);
        }
    }

    fclose(fp);
    fclose(tmp);
    rename(DATA_DIR "/bookings.tmp", path);
    unlock_file(fd);
    close(fd);

    if (flight_to_refund != -1) {
        // Refund seat in seat_manager if applicable
        if (strlen(seat_to_refund) > 0 && strcmp(seat_to_refund, "-") != 0) {
            cancel_booked_seat(flight_to_refund, seat_to_refund);
        }

        // Refund available count in flights.csv
        char fpath[100];
        snprintf(fpath, sizeof(fpath), "%s/flights.csv", DATA_DIR);
        int ffd = open(fpath, O_RDWR);
        if (ffd != -1 && lock_file(ffd, F_WRLCK) != -1) {
            FILE *ffp = fdopen(dup(ffd), "r");
            FILE *ftmp = fopen(DATA_DIR "/flights.tmp", "w");
            if (ffp && ftmp) {
                fgets(line, sizeof(line), ffp);
                fprintf(ftmp, "flight_id,origin,destination,total_seats,available_seats\n");
                while (fgets(line, sizeof(line), ffp)) {
                    int fid, tot, avail;
                    char orig[50], dest[50];
                    trim_string(line);
                    if (sscanf(line, "%d,%49[^,],%49[^,],%d,%d", &fid, orig, dest, &tot, &avail) == 5) {
                        if (fid == flight_to_refund) {
                            avail += seats_to_refund;
                            if (avail > tot) avail = tot;
                        }
                        fprintf(ftmp, "%d,%s,%s,%d,%d\n", fid, orig, dest, tot, avail);
                    }
                }
                fclose(ffp);
                fclose(ftmp);
                rename(DATA_DIR "/flights.tmp", fpath);
            }
            unlock_file(ffd);
            close(ffd);
        }
        send_message(sock, "Ticket cancelled successfully and seat released.\n");
    } else {
        send_message(sock, "Invalid booking ID or already cancelled.\n");
    }
}

static void view_my_bookings(int sock, int cid) {
    char path[100];
    snprintf(path, sizeof(path), "%s/bookings.csv", DATA_DIR);
    FILE *fp = fopen(path, "r");
    if (!fp) {
        send_message(sock, "No bookings found.\n");
        return;
    }

    char line[256], msg[BUFFER_SIZE];
    send_message(sock, "\n--- My Bookings ---\nID | Flight | Seat | Status | Date\n");
    fgets(line, sizeof(line), fp);

    while (fgets(line, sizeof(line), fp)) {
        int bid, cus_id, fid, seats;
        char seat_no[10] = "-", status[20], ts[50];
        trim_string(line);
        if (sscanf(line, "%d,%d,%d,%9[^,],%d,%19[^,],%49[^\n]", &bid, &cus_id, &fid, seat_no, &seats, status, ts) == 7) {
            if (cus_id == cid) {
                snprintf(msg, BUFFER_SIZE, "%d | F-%d | %s | %s | %s\n", bid, fid, seat_no, status, ts);
                send_message(sock, msg);
            }
        } else if (sscanf(line, "%d,%d,%d,%d,%19[^,],%49[^\n]", &bid, &cus_id, &fid, &seats, status, ts) == 6) {
            if (cus_id == cid) {
                snprintf(msg, BUFFER_SIZE, "%d | F-%d | - | %s | %s\n", bid, fid, status, ts);
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
    trim_string(buffer);
    if (strlen(buffer) == 0) {
        send_message(sock, "Password cannot be empty.\n");
        return;
    }

    char path[100];
    snprintf(path, sizeof(path), "%s/customers.csv", DATA_DIR);
    int fd = open(path, O_RDWR);
    if (fd == -1 || lock_file(fd, F_WRLCK) == -1) {
        if (fd != -1) close(fd);
        send_message(sock, "Error updating password.\n");
        return;
    }

    FILE *fp = fdopen(dup(fd), "r");
    FILE *tmp = fopen(DATA_DIR "/customers.tmp", "w");
    if (!fp || !tmp) {
        if (fp) fclose(fp);
        if (tmp) fclose(tmp);
        unlock_file(fd);
        close(fd);
        send_message(sock, "Internal Error.\n");
        return;
    }

    char hashed_pass[BCRYPT_HASHSIZE];
    bcrypt_hash(buffer, hashed_pass);

    char line[256];
    fgets(line, sizeof(line), fp);
    fprintf(tmp, "id,username,password,active\n");

    while (fgets(line, sizeof(line), fp)) {
        int id, active;
        char user[50], pass[70];
        trim_string(line);
        if (sscanf(line, "%d,%49[^,],%69[^,],%d", &id, user, pass, &active) == 4) {
            if (id == cid) {
                strncpy(pass, hashed_pass, sizeof(pass) - 1);
                pass[sizeof(pass) - 1] = '\0';
            }
            fprintf(tmp, "%d,%s,%s,%d\n", id, user, pass, active);
        }
    }
    fclose(fp);
    fclose(tmp);
    rename(DATA_DIR "/customers.tmp", path);
    unlock_file(fd);
    close(fd);
    send_message(sock, "Password updated successfully.\n");
}

static void add_feedback(int sock, int cid) {
    char buffer[BUFFER_SIZE];
    send_message(sock, "Enter your feedback: ");
    if (read_input(sock, buffer, BUFFER_SIZE)) {
        trim_string(buffer);
        if (strlen(buffer) == 0) {
            send_message(sock, "Feedback cannot be empty.\n");
            return;
        }

        char path[100];
        snprintf(path, sizeof(path), "%s/feedback.txt", DATA_DIR);
        FILE *fp = fopen(path, "a");
        if (fp) {
            fprintf(fp, "Passenger %d: %s\n", cid, buffer);
            fclose(fp);
            send_message(sock, "Feedback submitted successfully. Thank you!\n");
        } else {
            send_message(sock, "Error saving feedback.\n");
        }
    }
}

void handle_customer(int sock, int cid) {
    char buffer[BUFFER_SIZE];

    while (1) {
        send_message(sock,
            "\n=== Passenger Menu ===\n"
            "1. View Available Flights\n"
            "2. Book Ticket (Select & Lock Seat)\n"
            "3. Cancel Ticket\n"
            "4. View My Bookings\n"
            "5. Change Password\n"
            "6. Add Feedback\n"
            "7. Logout\n"
            "Choice: ");

        if (!read_input(sock, buffer, BUFFER_SIZE)) break;
        if (!is_numeric(buffer)) {
            send_message(sock, "Invalid choice! Please enter a number.\n");
            continue;
        }
        int choice = atoi(buffer);

        switch (choice) {
            case 1: view_available_flights(sock); break;
            case 2: book_ticket(sock, cid); break;
            case 3: cancel_ticket(sock, cid); break;
            case 4: view_my_bookings(sock, cid); break;
            case 5: change_password(sock, cid); break;
            case 6: add_feedback(sock, cid); break;
            case 7: return;
            default: send_message(sock, "Invalid choice! Please choose 1-7.\n");
        }
    }
}
