#include "auth.h"
#include "common.h"
#include "utils.h"
#include "session_manager.h"
#include "bcrypt.h"
#include <stdio.h>
#include <string.h>

static const char* get_csv_path(int role) {
    static char path[100];
    const char *files[] = {"", "customers.csv", "employees.csv", "managers.csv", "admins.csv"};
    if (role < 1 || role > 4) return "";
    snprintf(path, sizeof(path), "%s/%s", DATA_DIR, files[role]);
    return path;
}

int authenticate(int role, const char *username, const char *password, int *user_id) {
    const char *csv_path = get_csv_path(role);
    if (!csv_path || csv_path[0] == '\0') return 0;

    FILE *fp = fopen(csv_path, "r");
    if (!fp) {
        return 0;
    }

    char line[256];
    fgets(line, sizeof(line), fp); // Skip header

    while (fgets(line, sizeof(line), fp)) {
        int id;
        char user[50], pass[70];
        trim_string(line);

        if (role == ROLE_CUSTOMER) {
            int active;
            if (sscanf(line, "%d,%49[^,],%69[^,],%d", &id, user, pass, &active) == 4) {
                trim_string(user);
                trim_string(pass);
                if (strcmp(user, username) == 0 && bcrypt_checkpw(password, pass) == 0) {
                    if (!active) {
                        fclose(fp);
                        return -2; // Account deactivated
                    }
                    if (check_and_register_login(role, id)) {
                        *user_id = id;
                        fclose(fp);
                        return 1; // Success
                    } else {
                        fclose(fp);
                        return -1; // Already logged in
                    }
                }
            }
        } else {
            if (sscanf(line, "%d,%49[^,],%69[^\n]", &id, user, pass) == 3) {
                trim_string(user);
                trim_string(pass);
                if (strcmp(user, username) == 0 && bcrypt_checkpw(password, pass) == 0) {
                    if (check_and_register_login(role, id)) {
                        *user_id = id;
                        fclose(fp);
                        return 1; // Success
                    } else {
                        fclose(fp);
                        return -1; // Already logged in
                    }
                }
            }
        }
    }

    fclose(fp);
    return 0; // Invalid credentials
}

void logout_user(int role, int user_id) {
    register_logout(role, user_id);
}
