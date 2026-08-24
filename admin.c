#include "admin.h"
#include "common.h"
#include "utils.h"
#include "seat_manager.h"
#include "bcrypt.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

static void add_flight(int sock) {
    char buf[BUFFER_SIZE];
    char path[100];
    snprintf(path, sizeof(path), "%s/flights.csv", DATA_DIR);

    int file_exists = (access(path, F_OK) == 0);
    int new_id = get_next_id_csv(path);

    send_message(sock, "Enter Flight Origin: ");
    if (!read_input(sock, buf, BUFFER_SIZE)) return;
    trim_string(buf);
    if (!is_alpha(buf) || strlen(buf) == 0) {
        send_message(sock, "Invalid origin! Must be letters and spaces only.\n");
        return;
    }
    char origin[50];
    strncpy(origin, buf, sizeof(origin) - 1);
    origin[sizeof(origin) - 1] = '\0';

    send_message(sock, "Enter Flight Destination: ");
    if (!read_input(sock, buf, BUFFER_SIZE)) return;
    trim_string(buf);
    if (!is_alpha(buf) || strlen(buf) == 0) {
        send_message(sock, "Invalid destination! Must be letters and spaces only.\n");
        return;
    }
    char destination[50];
    strncpy(destination, buf, sizeof(destination) - 1);
    destination[sizeof(destination) - 1] = '\0';

    send_message(sock, "Enter Total Seats (e.g. 20, 60, 150): ");
    if (!read_input(sock, buf, BUFFER_SIZE)) return;
    if (!is_numeric(buf)) {
        send_message(sock, "Invalid seats! Must be a positive number.\n");
        return;
    }
    int total_seats = atoi(buf);
    if (total_seats <= 0 || total_seats > 500) {
        send_message(sock, "Invalid seats! Must be between 1 and 500.\n");
        return;
    }

    FILE *fp = fopen(path, "a");
    if (!fp) {
        send_message(sock, "Error opening flights file.\n");
        return;
    }

    if (!file_exists) {
        fprintf(fp, "flight_id,origin,destination,total_seats,available_seats\n");
    }

    fprintf(fp, "%d,%s,%s,%d,%d\n", new_id, origin, destination, total_seats, total_seats);
    fclose(fp);

    // Initialize individual seats for this flight in seats.csv
    init_flight_seats(new_id, total_seats);

    snprintf(buf, sizeof(buf), "✓ Flight %d (%s -> %s, %d seats) added successfully!\n",
             new_id, origin, destination, total_seats);
    send_message(sock, buf);
}

static void add_agent(int sock) {
    char buf[BUFFER_SIZE];
    char path[100];
    snprintf(path, sizeof(path), "%s/employees.csv", DATA_DIR);

    int file_exists = (access(path, F_OK) == 0);

    send_message(sock, "Enter agent username: ");
    if (!read_input(sock, buf, BUFFER_SIZE)) return;
    trim_string(buf);
    if (strlen(buf) == 0) {
        send_message(sock, "Username cannot be empty!\n");
        return;
    }
    if (!is_username_unique(path, buf)) {
        send_message(sock, "Agent username already exists! Please choose another.\n");
        return;
    }
    char username[50];
    strncpy(username, buf, sizeof(username) - 1);
    username[sizeof(username) - 1] = '\0';

    send_message(sock, "Enter agent password: ");
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
        send_message(sock, "Error opening agents file.\n");
        return;
    }

    if (!file_exists) {
        fprintf(fp, "id,username,password\n");
    }

    fprintf(fp, "%d,%s,%s\n", new_id, username, hashed_pass);
    fclose(fp);

    snprintf(buf, sizeof(buf), "✓ Agent created successfully! Assigned ID: %d\n", new_id);
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
        send_message(sock, "Internal error.\n");
        return;
    }

    char line[256];
    int found = 0;

    fgets(line, sizeof(line), fp); // header
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
        send_message(sock, "✓ Passenger modified successfully!\n");
    } else {
        remove(temp_path);
        send_message(sock, "Passenger not found.\n");
    }

    unlock_file(fd);
    close(fd);
}

static void modify_agent(int sock) {
    char buf[BUFFER_SIZE];
    send_message(sock, "Enter Agent ID to modify: ");
    if (!read_input(sock, buf, BUFFER_SIZE)) return;
    if (!is_numeric(buf)) {
        send_message(sock, "Invalid Agent ID! Must be a number.\n");
        return;
    }
    int eid = atoi(buf);

    char path[100], temp_path[100];
    snprintf(path, sizeof(path), "%s/employees.csv", DATA_DIR);
    snprintf(temp_path, sizeof(temp_path), "%s/employees.tmp", DATA_DIR);

    int fd = open(path, O_RDWR);
    if (fd == -1 || lock_file(fd, F_WRLCK) == -1) {
        if (fd != -1) close(fd);
        send_message(sock, "Error opening agents.\n");
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

    fgets(line, sizeof(line), fp);
    fprintf(tmp, "id,username,password\n");

    while (fgets(line, sizeof(line), fp)) {
        int id;
        char user[50], pass[70];
        trim_string(line);

        if (sscanf(line, "%d,%49[^,],%69[^\n]", &id, user, pass) == 3) {
            trim_string(user);
            trim_string(pass);

            if (id == eid) {
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
            fprintf(tmp, "%d,%s,%s\n", id, user, pass);
        }
    }

    fclose(fp);
    fclose(tmp);

    if (found) {
        rename(temp_path, path);
        send_message(sock, "✓ Agent modified successfully!\n");
    } else {
        remove(temp_path);
        send_message(sock, "Agent not found.\n");
    }

    unlock_file(fd);
    close(fd);
}

static void change_password(int sock, int admin_id) {
    char buf[BUFFER_SIZE];

    send_message(sock, "Enter new password: ");
    if (!read_input(sock, buf, BUFFER_SIZE)) return;
    trim_string(buf);
    if (strlen(buf) == 0) {
        send_message(sock, "Password cannot be empty.\n");
        return;
    }

    char hashed_pass[BCRYPT_HASHSIZE];
    bcrypt_hash(buf, hashed_pass);

    char path[100], temp_path[100];
    snprintf(path, sizeof(path), "%s/admins.csv", DATA_DIR);
    snprintf(temp_path, sizeof(temp_path), "%s/admins.tmp", DATA_DIR);

    int fd = open(path, O_RDWR);
    if (fd == -1 || lock_file(fd, F_WRLCK) == -1) {
        if (fd != -1) close(fd);
        send_message(sock, "Error opening admins.\n");
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
    fgets(line, sizeof(line), fp);
    fprintf(tmp, "id,username,password\n");

    while (fgets(line, sizeof(line), fp)) {
        int id;
        char user[50], pass[70];
        trim_string(line);

        if (sscanf(line, "%d,%49[^,],%69[^\n]", &id, user, pass) == 3) {
            if (id == admin_id) {
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

void handle_admin(int sock, int admin_id) {
    char buffer[BUFFER_SIZE];

    while (1) {
        send_message(sock,
            "\n=== Admin Menu ===\n"
            "1. Add New Flight\n"
            "2. Add New Agent\n"
            "3. Modify Passenger Details\n"
            "4. Modify Agent Details\n"
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
            case 1: add_flight(sock); break;
            case 2: add_agent(sock); break;
            case 3: modify_passenger(sock); break;
            case 4: modify_agent(sock); break;
            case 5: change_password(sock, admin_id); break;
            case 6: return;
            default: send_message(sock, "Invalid choice! Please choose 1-6.\n");
        }
    }
}
