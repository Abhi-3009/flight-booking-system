#include "utils.h"
#include "common.h"
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>

void send_message(int sock, const char *msg) {
    write(sock, msg, strlen(msg));
    usleep(1000);
}

int read_input(int sock, char *buffer, int size) {
    memset(buffer, 0, size);
    int n = read(sock, buffer, size);
    if (n > 0) {
        buffer[strcspn(buffer, "\n")] = 0;
        return 1;
    }
    return 0;
}

int get_next_id_csv(const char *csv_path) {
    FILE *fp = fopen(csv_path, "r");
    if (!fp) return 1;

    char line[512];
    int max_id = 0;

    fgets(line, sizeof(line), fp);

    while (fgets(line, sizeof(line), fp)) {
        int id;
        if (sscanf(line, "%d,", &id) == 1 && id > max_id) {
            max_id = id;
        }
    }

    fclose(fp);
    return max_id + 1;
}

int lock_file(int fd, int lock_type) {
    struct flock lock;
    lock.l_type = lock_type;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    lock.l_pid = getpid();

    if (fcntl(fd, F_SETLKW, &lock) == -1) {
        perror("Error acquiring lock");
        return -1;
    }
    return 0;
}

int unlock_file(int fd) {
    struct flock lock;
    lock.l_type = F_UNLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    lock.l_pid = getpid();

    if (fcntl(fd, F_SETLK, &lock) == -1) {
        perror("Error releasing lock");
        return -1;
    }
    return 0;
}

// NEW: Trim whitespace from strings
void trim_string(char *str) {
    int len = strlen(str);
    while (len > 0 && (str[len-1] == ' ' || str[len-1] == '\t' ||
                       str[len-1] == '\n' || str[len-1] == '\r')) {
        str[len-1] = '\0';
        len--;
    }
}

int is_numeric(const char *str) {
    if (!str || *str == '\0') return 0;
    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] < '0' || str[i] > '9') return 0;
    }
    return 1;
}

int is_alpha(const char *str) {
    if (!str || *str == '\0') return 0;
    for (int i = 0; str[i] != '\0'; i++) {
        if ((str[i] < 'A' || str[i] > 'Z') && 
            (str[i] < 'a' || str[i] > 'z') && 
            str[i] != ' ') {
            return 0;
        }
    }
    return 1;
}
