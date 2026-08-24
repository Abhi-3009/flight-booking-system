#include "employee.h"
#include "common.h"
#include "utils.h"
#include "bcrypt.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

static void add_passenger(int sock) {
    char buf[BUFFER_SIZE];
    char path[100];
    snprintf(path, sizeof(path), "%s/customers.csv", DATA_DIR);

    send_message(sock, "Enter username: ");
    if (!read_input(sock, buf, BUFFER_SIZE)) return;
    trim_string(buf);
    if (strlen(buf) == 0) {
        send_message(sock, "Username cannot be empty!\n");
        return;
    }
    if (!is_username_unique(path, buf)) {
        send_message(sock, "Username already exists! Please choose another.\n");
        return;
    }
    char username[50];
    strncpy(username, buf, sizeof(username) - 1);
    username[sizeof(username) - 1] = '\0';

    send_message(sock, "Enter password: ");
    if (!read_input(sock, buf, BUFFER_SIZE)) return;
    trim_string(buf);
    if (strlen(buf) == 0) {
        send_message(sock, "Password cannot be empty!\n");
        return;
    }
    char password[50];
    strncpy(password, buf, sizeof(password) - 1);
    password[sizeof(password) - 1] = '\0';

    char hashed_pass[BCRYPT_HASHSIZE];
    bcrypt_hash(password, hashed_pass);

    int new_id = get_next_id_csv(path);

    FILE *fp = fopen(path, "a");
    if (!fp) {
        send_message(sock, "Error opening database.\n");
        return;
    }

    fprintf(fp, "%d,%s,%s,1\n", new_id, username, hashed_pass);
    fclose(fp);

    snprintf(buf, sizeof(buf), "✓ Passenger created successfully! Assigned ID: %d\n", new_id);
    send_message(sock, buf);
}

static void modify_passenger(int sock) {
    char buf[BUFFER_SIZE];

    send_message(sock, "Enter Passenger ID to modify: ");
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
        send_message(sock, "Error opening passengers.\n");
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

    char line[256];
    int found = 0;

    fgets(line, sizeof(line), fp);
    fprintf(tmp, "id,username,password,active\n");

    while (fgets(line, sizeof(line), fp)) {
        int id, active;
        char user[50], pass[70];
        trim_string(line);

        if (sscanf(line, "%d,%49[^,],%69[^,],%d", &id, user, pass, &active) == 4) {
            trim_string(user);
            trim_string(pass);

            if (id == cid) {
                found = 1;

                send_message(sock, "New username (or '-' to skip): ");
                if (read_input(sock, buf, BUFFER_SIZE)) {
                    trim_string(buf);
                    if (strcmp(buf, "-") != 0 && strlen(buf) > 0) {
                        if (strcmp(user, buf) != 0 && !is_username_unique(path, buf)) {
                            send_message(sock, "Warning: Username already exists, skipping username update.\n");
                        } else {
                            strncpy(user, buf, sizeof(user) - 1);
                            user[sizeof(user) - 1] = '\0';
                        }
                    }
                }

                send_message(sock, "New password (or '-' to skip): ");
                if (read_input(sock, buf, BUFFER_SIZE)) {
                    trim_string(buf);
                    if (strcmp(buf, "-") != 0 && strlen(buf) > 0) {
                        char new_hashed[BCRYPT_HASHSIZE];
                        bcrypt_hash(buf, new_hashed);
                        strncpy(pass, new_hashed, sizeof(pass) - 1);
                        pass[sizeof(pass) - 1] = '\0';
                    }
                }
            }
            fprintf(tmp, "%d,%s,%s,%d\n", id, user, pass, active);
        }
    }

    fclose(fp);
    fclose(tmp);

    if (found) {
        rename(temp_path, path);
        send_message(sock, "✓ Passenger details updated successfully!\n");
    } else {
        remove(temp_path);
        send_message(sock, "Passenger ID not found.\n");
    }

    unlock_file(fd);
    close(fd);
}

static void view_all_flights(int sock) {
    FILE *fp = fopen(DATA_DIR "/flights.csv", "r");
    if (!fp) {
        send_message(sock, "No flights available.\n");
        return;
    }

    char line[256], msg[BUFFER_SIZE];
    strcpy(msg, "\n--- All Flights ---\nID | Origin -> Destination | Total | Available\n");
    send_message(sock, msg);

    fgets(line, sizeof(line), fp); // Header
    while (fgets(line, sizeof(line), fp)) {
        int fid, tot, avail;
        char orig[50], dest[50];
        trim_string(line);
        if (sscanf(line, "%d,%49[^,],%49[^,],%d,%d", &fid, orig, dest, &tot, &avail) == 5) {
            snprintf(msg, BUFFER_SIZE, "%d | %s -> %s | %d | %d seats\n", fid, orig, dest, tot, avail);
            send_message(sock, msg);
        }
    }
    fclose(fp);
}

static void view_passenger_bookings(int sock) {
    char buf[BUFFER_SIZE];
    send_message(sock, "Enter Passenger ID: ");
    if (!read_input(sock, buf, BUFFER_SIZE)) return;
    if (!is_numeric(buf)) {
        send_message(sock, "Invalid Passenger ID! Must be a number.\n");
        return;
    }
    int target_cid = atoi(buf);

    FILE *fp = fopen(DATA_DIR "/bookings.csv", "r");
    if (!fp) {
        send_message(sock, "No bookings found.\n");
        return;
    }

    char line[256], msg[BUFFER_SIZE];
    send_message(sock, "\n--- Passenger Bookings ---\nID | Flight | Seat | Status | Date\n");
    fgets(line, sizeof(line), fp);

    int count = 0;
    while (fgets(line, sizeof(line), fp)) {
        int bid, cus_id, fid, seats;
        char seat_no[10] = "-", status[20], ts[50];
        trim_string(line);
        if (sscanf(line, "%d,%d,%d,%9[^,],%d,%19[^,],%49[^\n]", &bid, &cus_id, &fid, seat_no, &seats, status, ts) == 7) {
            if (cus_id == target_cid) {
                snprintf(msg, BUFFER_SIZE, "%d | F-%d | %s | %s | %s\n", bid, fid, seat_no, status, ts);
                send_message(sock, msg);
                count++;
            }
        } else if (sscanf(line, "%d,%d,%d,%d,%19[^,],%49[^\n]", &bid, &cus_id, &fid, &seats, status, ts) == 6) {
            if (cus_id == target_cid) {
                snprintf(msg, BUFFER_SIZE, "%d | F-%d | - | %s | %s\n", bid, fid, status, ts);
                send_message(sock, msg);
                count++;
            }
        }
    }
    fclose(fp);

    if (count == 0) {
        send_message(sock, "No bookings found for this passenger.\n");
    }
}

static void change_password(int sock, int eid) {
    char buf[BUFFER_SIZE];

    send_message(sock, "Enter new password: ");
    if (!read_input(sock, buf, BUFFER_SIZE)) return;
    trim_string(buf);
    if (strlen(buf) == 0) {
        send_message(sock, "Password cannot be empty.\n");
        return;
    }

    char path[100], temp_path[100];
    snprintf(path, sizeof(path), "%s/employees.csv", DATA_DIR);
    snprintf(temp_path, sizeof(temp_path), "%s/employees.tmp", DATA_DIR);

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
            if (id == eid) {
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

void handle_employee(int sock, int eid) {
    char buffer[BUFFER_SIZE];

    while (1) {
        send_message(sock,
            "\n=== Agent Menu ===\n"
            "1. Add New Passenger\n"
            "2. Modify Passenger Details\n"
            "3. View All Flights\n"
            "4. View Passenger Bookings\n"
            "5. Change Password\n"
            "6. Logout\n"
            "Choice: ");

        if (!read_input(sock, buffer, BUFFER_SIZE)) break;
        if (!is_numeric(buffer)) {
            send_message(sock, "Invalid choice! Please enter a number.\n");
            continue;
        }
        int choice = atoi(buffer);

        switch (choice) {
            case 1: add_passenger(sock); break;
            case 2: modify_passenger(sock); break;
            case 3: view_all_flights(sock); break;
            case 4: view_passenger_bookings(sock); break;
            case 5: change_password(sock, eid); break;
            case 6: return;
            default: send_message(sock, "Invalid choice! Please choose 1-6.\n");
        }
    }
}
