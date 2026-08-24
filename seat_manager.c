#include "seat_manager.h"
#include "utils.h"
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>

static SeatHold active_holds[MAX_ACTIVE_HOLDS];
static pthread_mutex_t seat_mutex = PTHREAD_MUTEX_INITIALIZER;

static void clean_expired_holds_unlocked() {
    time_t now = time(NULL);
    for (int i = 0; i < MAX_ACTIVE_HOLDS; i++) {
        if (active_holds[i].hold_expires_at > 0 && active_holds[i].hold_expires_at <= now) {
            memset(&active_holds[i], 0, sizeof(SeatHold));
        }
    }
}

void init_seat_manager() {
    pthread_mutex_lock(&seat_mutex);
    memset(active_holds, 0, sizeof(active_holds));
    pthread_mutex_unlock(&seat_mutex);

    char path[100];
    snprintf(path, sizeof(path), "%s/seats.csv", DATA_DIR);
    if (access(path, F_OK) != 0) {
        FILE *fp = fopen(path, "w");
        if (fp) {
            fprintf(fp, "flight_id,seat_number,status\n");
            fclose(fp);
        }
    }
    printf("✓ Seat manager initialized\n");
}

int init_flight_seats(int flight_id, int total_seats) {
    char path[100];
    snprintf(path, sizeof(path), "%s/seats.csv", DATA_DIR);

    // Check if seats already exist for this flight
    FILE *fp = fopen(path, "r");
    if (fp) {
        char line[256];
        fgets(line, sizeof(line), fp); // Header
        while (fgets(line, sizeof(line), fp)) {
            int fid;
            if (sscanf(line, "%d,", &fid) == 1 && fid == flight_id) {
                fclose(fp);
                return 1; // Already initialized
            }
        }
        fclose(fp);
    }

    // Append seats for this flight (4 seats per row: A, B, C, D)
    fp = fopen(path, "a");
    if (!fp) return 0;

    for (int i = 0; i < total_seats; i++) {
        int row = (i / 4) + 1;
        char col = 'A' + (i % 4);
        fprintf(fp, "%d,%d%c,AVAILABLE\n", flight_id, row, col);
    }

    fclose(fp);
    return 1;
}

void display_seat_map(int sock, int flight_id) {
    char path[100];
    snprintf(path, sizeof(path), "%s/seats.csv", DATA_DIR);

    // Ensure seats exist
    FILE *fp = fopen(path, "r");
    if (!fp) {
        send_message(sock, "Unable to load seat map.\n");
        return;
    }

    char line[256];
    int count = 0;
    fgets(line, sizeof(line), fp);
    while (fgets(line, sizeof(line), fp)) {
        int fid;
        if (sscanf(line, "%d,", &fid) == 1 && fid == flight_id) {
            count++;
        }
    }
    fclose(fp);

    if (count == 0) {
        // Look up total seats from flights.csv
        char fpath[100];
        snprintf(fpath, sizeof(fpath), "%s/flights.csv", DATA_DIR);
        FILE *ffp = fopen(fpath, "r");
        int tot = 20;
        if (ffp) {
            fgets(line, sizeof(line), ffp);
            while (fgets(line, sizeof(line), ffp)) {
                int fid, total_s, avail_s;
                char orig[50], dest[50];
                trim_string(line);
                if (sscanf(line, "%d,%49[^,],%49[^,],%d,%d", &fid, orig, dest, &total_s, &avail_s) == 5) {
                    if (fid == flight_id) {
                        tot = total_s;
                        break;
                    }
                }
            }
            fclose(ffp);
        }
        init_flight_seats(flight_id, tot);
    }

    pthread_mutex_lock(&seat_mutex);
    clean_expired_holds_unlocked();
    time_t now = time(NULL);

    fp = fopen(path, "r");
    if (!fp) {
        pthread_mutex_unlock(&seat_mutex);
        send_message(sock, "Error opening seats file.\n");
        return;
    }

    char header[BUFFER_SIZE];
    snprintf(header, sizeof(header),
        "\n======================= FLIGHT %d SEAT MAP =======================\n"
        "  Col A      Col B          Col C      Col D\n"
        "---------------------------------------------------------------\n",
        flight_id);
    send_message(sock, header);

    char row_buffer[BUFFER_SIZE] = "";
    int current_row = -1;
    int col_index = 0;

    fgets(line, sizeof(line), fp); // Header
    while (fgets(line, sizeof(line), fp)) {
        int fid;
        char seat_no[10], status[20];
        trim_string(line);
        if (sscanf(line, "%d,%9[^,],%19[^\n]", &fid, seat_no, status) == 3) {
            if (fid != flight_id) continue;

            int row = atoi(seat_no);
            if (row != current_row) {
                if (current_row != -1) {
                    strcat(row_buffer, "\n");
                    send_message(sock, row_buffer);
                    row_buffer[0] = '\0';
                }
                current_row = row;
                col_index = 0;
            }

            // Check in-memory hold
            int is_held = 0;
            for (int i = 0; i < MAX_ACTIVE_HOLDS; i++) {
                if (active_holds[i].flight_id == flight_id &&
                    strcmp(active_holds[i].seat_number, seat_no) == 0 &&
                    active_holds[i].hold_expires_at > now) {
                    is_held = 1;
                    break;
                }
            }

            char seat_display[64];
            if (strcmp(status, "BOOKED") == 0) {
                snprintf(seat_display, sizeof(seat_display), "[%s:TAKEN] ", seat_no);
            } else if (is_held) {
                snprintf(seat_display, sizeof(seat_display), "[%s:HELD]  ", seat_no);
            } else {
                snprintf(seat_display, sizeof(seat_display), "[%s:FREE]  ", seat_no);
            }

            if (col_index == 2) {
                strcat(row_buffer, "    "); // Aisle space
            }
            strcat(row_buffer, seat_display);
            col_index++;
        }
    }
    if (strlen(row_buffer) > 0) {
        strcat(row_buffer, "\n");
        send_message(sock, row_buffer);
    }

    fclose(fp);
    pthread_mutex_unlock(&seat_mutex);

    send_message(sock,
        "---------------------------------------------------------------\n"
        "Legend: [FREE] = Available | [HELD] = Reserved (2 min) | [TAKEN] = Booked\n"
        "===============================================================\n\n");
}

int try_hold_seat(int flight_id, const char *seat_number, int customer_id, char *err_msg, int err_msg_len) {
    pthread_mutex_lock(&seat_mutex);
    clean_expired_holds_unlocked();
    time_t now = time(NULL);

    char path[100];
    snprintf(path, sizeof(path), "%s/seats.csv", DATA_DIR);
    FILE *fp = fopen(path, "r");
    if (!fp) {
        pthread_mutex_unlock(&seat_mutex);
        snprintf(err_msg, err_msg_len, "Internal error: Cannot access seat records.\n");
        return 0;
    }

    char line[256];
    int seat_found = 0;
    int is_booked = 0;
    fgets(line, sizeof(line), fp);
    while (fgets(line, sizeof(line), fp)) {
        int fid;
        char seat_no[10], status[20];
        trim_string(line);
        if (sscanf(line, "%d,%9[^,],%19[^\n]", &fid, seat_no, status) == 3) {
            if (fid == flight_id && strcasecmp(seat_no, seat_number) == 0) {
                seat_found = 1;
                if (strcmp(status, "BOOKED") == 0) {
                    is_booked = 1;
                }
                break;
            }
        }
    }
    fclose(fp);

    if (!seat_found) {
        pthread_mutex_unlock(&seat_mutex);
        snprintf(err_msg, err_msg_len, "Seat %s does not exist on Flight %d.\n", seat_number, flight_id);
        return 0;
    }

    if (is_booked) {
        pthread_mutex_unlock(&seat_mutex);
        snprintf(err_msg, err_msg_len, "Seat %s is already BOOKED.\n", seat_number);
        return 0;
    }

    // Check if currently held
    for (int i = 0; i < MAX_ACTIVE_HOLDS; i++) {
        if (active_holds[i].flight_id == flight_id &&
            strcasecmp(active_holds[i].seat_number, seat_number) == 0 &&
            active_holds[i].hold_expires_at > now) {
            if (active_holds[i].customer_id == customer_id) {
                // Customer already holds this seat; refresh hold timer
                active_holds[i].hold_expires_at = now + HOLD_TIMEOUT_SECONDS;
                pthread_mutex_unlock(&seat_mutex);
                return 1;
            } else {
                int secs_left = (int)(active_holds[i].hold_expires_at - now);
                pthread_mutex_unlock(&seat_mutex);
                snprintf(err_msg, err_msg_len, "Seat %s is currently locked by another passenger (expires in %d seconds).\n", seat_number, secs_left);
                return 0;
            }
        }
    }

    // Find free slot in active_holds
    for (int i = 0; i < MAX_ACTIVE_HOLDS; i++) {
        if (active_holds[i].hold_expires_at == 0) {
            active_holds[i].flight_id = flight_id;
            strncpy(active_holds[i].seat_number, seat_number, sizeof(active_holds[i].seat_number) - 1);
            active_holds[i].customer_id = customer_id;
            active_holds[i].hold_expires_at = now + HOLD_TIMEOUT_SECONDS;
            pthread_mutex_unlock(&seat_mutex);
            return 1;
        }
    }

    pthread_mutex_unlock(&seat_mutex);
    snprintf(err_msg, err_msg_len, "System busy: maximum concurrent seat holds reached.\n");
    return 0;
}

int confirm_seat_booking(int flight_id, const char *seat_number, int customer_id) {
    pthread_mutex_lock(&seat_mutex);
    clean_expired_holds_unlocked();
    time_t now = time(NULL);

    // Verify hold
    int hold_idx = -1;
    for (int i = 0; i < MAX_ACTIVE_HOLDS; i++) {
        if (active_holds[i].flight_id == flight_id &&
            strcasecmp(active_holds[i].seat_number, seat_number) == 0 &&
            active_holds[i].customer_id == customer_id &&
            active_holds[i].hold_expires_at > now) {
            hold_idx = i;
            break;
        }
    }

    if (hold_idx == -1) {
        pthread_mutex_unlock(&seat_mutex);
        return 0; // Hold expired or not found
    }

    // Update status in data/seats.csv
    char path[100], temp_path[100];
    snprintf(path, sizeof(path), "%s/seats.csv", DATA_DIR);
    snprintf(temp_path, sizeof(temp_path), "%s/seats.tmp", DATA_DIR);

    FILE *fp = fopen(path, "r");
    FILE *tmp = fopen(temp_path, "w");
    if (!fp || !tmp) {
        if (fp) fclose(fp);
        if (tmp) fclose(tmp);
        pthread_mutex_unlock(&seat_mutex);
        return 0;
    }

    char line[256];
    fgets(line, sizeof(line), fp);
    fprintf(tmp, "flight_id,seat_number,status\n");

    int updated = 0;
    while (fgets(line, sizeof(line), fp)) {
        int fid;
        char seat_no[10], status[20];
        trim_string(line);
        if (sscanf(line, "%d,%9[^,],%19[^\n]", &fid, seat_no, status) == 3) {
            if (fid == flight_id && strcasecmp(seat_no, seat_number) == 0) {
                strcpy(status, "BOOKED");
                updated = 1;
            }
            fprintf(tmp, "%d,%s,%s\n", fid, seat_no, status);
        }
    }

    fclose(fp);
    fclose(tmp);

    if (updated) {
        rename(temp_path, path);
        memset(&active_holds[hold_idx], 0, sizeof(SeatHold));
        pthread_mutex_unlock(&seat_mutex);
        return 1;
    } else {
        remove(temp_path);
        pthread_mutex_unlock(&seat_mutex);
        return 0;
    }
}

void release_seat_hold(int flight_id, const char *seat_number, int customer_id) {
    pthread_mutex_lock(&seat_mutex);
    for (int i = 0; i < MAX_ACTIVE_HOLDS; i++) {
        if (active_holds[i].flight_id == flight_id &&
            strcasecmp(active_holds[i].seat_number, seat_number) == 0 &&
            active_holds[i].customer_id == customer_id) {
            memset(&active_holds[i], 0, sizeof(SeatHold));
            break;
        }
    }
    pthread_mutex_unlock(&seat_mutex);
}

void release_customer_holds(int customer_id) {
    pthread_mutex_lock(&seat_mutex);
    for (int i = 0; i < MAX_ACTIVE_HOLDS; i++) {
        if (active_holds[i].customer_id == customer_id) {
            memset(&active_holds[i], 0, sizeof(SeatHold));
        }
    }
    pthread_mutex_unlock(&seat_mutex);
}

int cancel_booked_seat(int flight_id, const char *seat_number) {
    char path[100], temp_path[100];
    snprintf(path, sizeof(path), "%s/seats.csv", DATA_DIR);
    snprintf(temp_path, sizeof(temp_path), "%s/seats.tmp", DATA_DIR);

    pthread_mutex_lock(&seat_mutex);

    FILE *fp = fopen(path, "r");
    FILE *tmp = fopen(temp_path, "w");
    if (!fp || !tmp) {
        if (fp) fclose(fp);
        if (tmp) fclose(tmp);
        pthread_mutex_unlock(&seat_mutex);
        return 0;
    }

    char line[256];
    fgets(line, sizeof(line), fp);
    fprintf(tmp, "flight_id,seat_number,status\n");

    int updated = 0;
    while (fgets(line, sizeof(line), fp)) {
        int fid;
        char seat_no[10], status[20];
        trim_string(line);
        if (sscanf(line, "%d,%9[^,],%19[^\n]", &fid, seat_no, status) == 3) {
            if (fid == flight_id && strcasecmp(seat_no, seat_number) == 0) {
                strcpy(status, "AVAILABLE");
                updated = 1;
            }
            fprintf(tmp, "%d,%s,%s\n", fid, seat_no, status);
        }
    }

    fclose(fp);
    fclose(tmp);

    if (updated) {
        rename(temp_path, path);
    } else {
        remove(temp_path);
    }

    pthread_mutex_unlock(&seat_mutex);
    return updated;
}
