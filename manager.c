#include "manager.h"
#include "common.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

static void activate_passenger(int sock) {
    char buf[BUFFER_SIZE];
    send_message(sock, "Enter Passenger ID to activate/deactivate: ");
    if (!read_input(sock, buf, BUFFER_SIZE)) return;
    int cid = atoi(buf);

    char path[100], temp_path[100];
    snprintf(path, 100, "%s/customers.csv", DATA_DIR);
    snprintf(temp_path, 100, "%s/customers.tmp", DATA_DIR);

    int fd = open(path, O_RDWR);
    if (fd == -1 || lock_file(fd, F_WRLCK) == -1) {
        if (fd != -1) close(fd);
        send_message(sock, "Error\n");
        return;
    }

    FILE *fp = fdopen(dup(fd), "r");
    FILE *tmp = fopen(temp_path, "w");

    char line[256];
    int found = 0;

    fgets(line, sizeof(line), fp);
    fprintf(tmp, "id,username,password,active\n");

    while (fgets(line, sizeof(line), fp)) {
        int id, active;
        char user[50], pass[50];

        trim_string(line);

        if (sscanf(line, "%d,%49[^,],%49[^,],%d", &id, user, pass, &active) == 4) {
            if (id == cid) {
                found = 1;
                active = !active;
            }
            fprintf(tmp, "%d,%s,%s,%d\n", id, user, pass, active);
        }
    }

    fclose(fp); fclose(tmp);

    if (found) {
        rename(temp_path, path);
        send_message(sock, "Passenger status updated!\n");
    } else {
        remove(temp_path);
        send_message(sock, "Passenger not found.\n");
    }
    
    unlock_file(fd); close(fd);
}

static void view_all_bookings(int sock) {
    FILE *fp = fopen(DATA_DIR "/bookings.csv", "r");
    if (!fp) { send_message(sock, "No bookings.\n"); return; }
    
    char line[256], msg[BUFFER_SIZE];
    send_message(sock, "\n--- All Bookings ---\nID | Pax | Flight | Seats | Status | Date\n");
    fgets(line, sizeof(line), fp);
    
    while (fgets(line, sizeof(line), fp)) {
        int bid, cus_id, fid, seats;
        char status[20], ts[50];
        trim_string(line);
        if (sscanf(line, "%d,%d,%d,%d,%19[^,],%49[^\n]", &bid, &cus_id, &fid, &seats, status, ts) == 6) {
            snprintf(msg, BUFFER_SIZE, "%d | %d | F-%d | %d seats | %s | %s\n", bid, cus_id, fid, seats, status, ts);
            send_message(sock, msg);
        }
    }
    fclose(fp);
}

static void view_feedback(int sock) {
    FILE *fp = fopen(DATA_DIR "/feedback.txt", "r");
    if (!fp) {
        send_message(sock, "No feedback available.\n");
        return;
    }

    char line[512];
    send_message(sock, "\n=== Customer Feedback ===\n");
    while (fgets(line, sizeof(line), fp)) {
        send_message(sock, line);
    }
    fclose(fp);
}

static void change_password(int sock, int mid) {
    char buf[BUFFER_SIZE];

    send_message(sock, "Enter new password: ");
    if (!read_input(sock, buf, BUFFER_SIZE)) return;

    char path[100], temp_path[100];
    snprintf(path, 100, "%s/managers.csv", DATA_DIR);
    snprintf(temp_path, 100, "%s/managers.tmp", DATA_DIR);

    int fd = open(path, O_RDWR);
    if (fd == -1 || lock_file(fd, F_WRLCK) == -1) {
        if (fd != -1) close(fd);
        send_message(sock, "Error\n");
        return;
    }

    FILE *fp = fdopen(dup(fd), "r");
    FILE *tmp = fopen(temp_path, "w");

    char line[256];
    fgets(line, sizeof(line), fp);
    fprintf(tmp, "id,username,password\n");

    while (fgets(line, sizeof(line), fp)) {
        int id;
        char user[50], pass[50];
        trim_string(line);
        if (sscanf(line, "%d,%49[^,],%49[^\n]", &id, user, pass) == 3) {
            if (id == mid) strcpy(pass, buf);
            fprintf(tmp, "%d,%s,%s\n", id, user, pass);
        }
    }

    fclose(fp); fclose(tmp);
    rename(temp_path, path);
    unlock_file(fd); close(fd);

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
        if (!is_numeric(buffer)) { send_message(sock, "Invalid choice! Please enter a number.\n"); continue; }
        int choice = atoi(buffer);

        switch (choice) {
            case 1: activate_passenger(sock); break;
            case 2: view_all_bookings(sock); break;
            case 3: view_feedback(sock); break;
            case 4: change_password(sock, mid); break;
            case 5: return;
            default: send_message(sock, "Invalid choice\n");
        }
    }
}
