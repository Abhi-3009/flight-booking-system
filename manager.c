#include "manager.h"
#include "common.h"
#include "utils.h"
#include "bcrypt.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

static void activate_passenger(int sock) {
    char buf[BUFFER_SIZE];
    send_message(sock, "Enter Passenger ID to toggle active status: ");
    if (!read_input(sock, buf, BUFFER_SIZE)) return;
    if (!is_numeric(buf)) {
        send_message(sock, "Invalid Passenger ID! Must be a number.\n");
        return;
    }
    int cid = atoi(buf);

    char path[100], temp_path[100];
    snprintf(path, sizeof(path), "%s/customers.csv", DATA_DIR);
    snprintf(temp_path, sizeof(temp_path), "%s/customers.tmp", DATA_DIR);

    int fd = open(path, O_RDWR);
    if (fd == -1 || lock_file(fd, F_WRLCK) == -1) {
        if (fd != -1) close(fd);
        send_message(sock, "Error opening passengers file.\n");
        return;
    }

    FILE *fp = fdopen(dup(fd), "r");
    FILE *tmp = fopen(temp_path, "w");
    if (!fp || !tmp) {
        if (fp) fclose(fp);
        if (tmp) fclose(tmp);
        unlock_file(fd);
        close(fd);
        send_message(sock, "Internal error.\n");
        return;
    }

    char line[256];
    int found = 0;
    int new_status = 0;

    fgets(line, sizeof(line), fp);
    fprintf(tmp, "id,username,password,active\n");

    while (fgets(line, sizeof(line), fp)) {
        int id, active;
        char user[50], pass[70];
        trim_string(line);

        if (sscanf(line, "%d,%49[^,],%69[^,],%d", &id, user, pass, &active) == 4) {
            if (id == cid) {
                found = 1;
                active = !active;
                new_status = active;
            }
            fprintf(tmp, "%d,%s,%s,%d\n", id, user, pass, active);
        }
    }

    fclose(fp);
    fclose(tmp);

    if (found) {
        rename(temp_path, path);
        if (new_status) {
            send_message(sock, "✓ Passenger account ACTIVATED successfully!\n");
        } else {
            send_message(sock, "✓ Passenger account DEACTIVATED successfully!\n");
        }
    } else {
        remove(temp_path);
        send_message(sock, "Passenger ID not found.\n");
    }

    unlock_file(fd);
    close(fd);
}

static void view_all_bookings(int sock) {
    FILE *fp = fopen(DATA_DIR "/bookings.csv", "r");
    if (!fp) {
        send_message(sock, "No bookings found.\n");
        return;
    }

    char line[256], msg[BUFFER_SIZE];
    send_message(sock, "\n--- All System Bookings ---\nID | Pax | Flight | Seat | Status | Date\n");
    fgets(line, sizeof(line), fp);

    int count = 0;
    while (fgets(line, sizeof(line), fp)) {
        int bid, cus_id, fid, seats;
        char seat_no[10] = "-", status[20], ts[50];
        trim_string(line);
        if (sscanf(line, "%d,%d,%d,%9[^,],%d,%19[^,],%49[^\n]", &bid, &cus_id, &fid, seat_no, &seats, status, ts) == 7) {
            snprintf(msg, BUFFER_SIZE, "%d | %d | F-%d | %s | %s | %s\n", bid, cus_id, fid, seat_no, status, ts);
            send_message(sock, msg);
            count++;
        } else if (sscanf(line, "%d,%d,%d,%d,%19[^,],%49[^\n]", &bid, &cus_id, &fid, &seats, status, ts) == 6) {
            snprintf(msg, BUFFER_SIZE, "%d | %d | F-%d | - | %s | %s\n", bid, cus_id, fid, status, ts);
            send_message(sock, msg);
            count++;
        }
    }
    fclose(fp);

    if (count == 0) {
        send_message(sock, "No booking records found.\n");
    }
}

static void view_feedback(int sock) {
    FILE *fp = fopen(DATA_DIR "/feedback.txt", "r");
    if (!fp) {
        send_message(sock, "No feedback available.\n");
        return;
    }

    char line[512];
    send_message(sock, "\n=== Customer Feedback Records ===\n");
    int count = 0;
    while (fgets(line, sizeof(line), fp)) {
        send_message(sock, line);
        count++;
    }
    fclose(fp);

    if (count == 0) {
        send_message(sock, "No feedback submitted yet.\n");
    }
}

static void change_password(int sock, int mid) {
    char buf[BUFFER_SIZE];

    send_message(sock, "Enter new password: ");
    if (!read_input(sock, buf, BUFFER_SIZE)) return;
    trim_string(buf);
    if (strlen(buf) == 0) {
        send_message(sock, "Password cannot be empty.\n");
        return;
    }

    char path[100], temp_path[100];
    snprintf(path, sizeof(path), "%s/managers.csv", DATA_DIR);
    snprintf(temp_path, sizeof(temp_path), "%s/managers.tmp", DATA_DIR);

    int fd = open(path, O_RDWR);
    if (fd == -1 || lock_file(fd, F_WRLCK) == -1) {
        if (fd != -1) close(fd);
        send_message(sock, "Error updating password.\n");
        return;
    }

    FILE *fp = fdopen(dup(fd), "r");
    FILE *tmp = fopen(temp_path, "w");
    if (!fp || !tmp) {
        if (fp) fclose(fp);
        if (tmp) fclose(tmp);
        unlock_file(fd);
        close(fd);
        send_message(sock, "Internal Error.\n");
        return;
    }

    char hashed_pass[BCRYPT_HASHSIZE];
    bcrypt_hash(buf, hashed_pass);

    char line[256];
    fgets(line, sizeof(line), fp);
    fprintf(tmp, "id,username,password\n");

    while (fgets(line, sizeof(line), fp)) {
        int id;
        char user[50], pass[70];
        trim_string(line);
        if (sscanf(line, "%d,%49[^,],%69[^\n]", &id, user, pass) == 3) {
            if (id == mid) {
                strncpy(pass, hashed_pass, sizeof(pass) - 1);
                pass[sizeof(pass) - 1] = '\0';
            }
            fprintf(tmp, "%d,%s,%s\n", id, user, pass);
        }
    }

    fclose(fp);
    fclose(tmp);
    rename(temp_path, path);
    unlock_file(fd);
    close(fd);

    send_message(sock, "Password changed successfully!\n");
}

void handle_manager(int sock, int mid) {
    char buffer[BUFFER_SIZE];

    while (1) {
        send_message(sock,
            "\n=== Manager Menu ===\n"
            "1. Activate/Deactivate Passenger\n"
            "2. View All Bookings\n"
            "3. View Customer Feedback\n"
            "4. Change Password\n"
            "5. Logout\n"
            "Choice: ");

        if (!read_input(sock, buffer, BUFFER_SIZE)) break;
        if (!is_numeric(buffer)) {
            send_message(sock, "Invalid choice! Please enter a number.\n");
            continue;
        }
        int choice = atoi(buffer);

        switch (choice) {
            case 1: activate_passenger(sock); break;
            case 2: view_all_bookings(sock); break;
            case 3: view_feedback(sock); break;
            case 4: change_password(sock, mid); break;
            case 5: return;
            default: send_message(sock, "Invalid choice! Please choose 1-5.\n");
        }
    }
}
