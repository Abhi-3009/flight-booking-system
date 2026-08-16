#include "auth.h"
#include "common.h"
#include "utils.h"
#include "session_manager.h"
#include <stdio.h>
#include <string.h>

static const char* get_csv_path(int role) {
    static char path[100];
    const char *files[] = {"", "customers.csv", "employees.csv", "managers.csv", "admins.csv"};
    snprintf(path, 100, "%s/%s", DATA_DIR, files[role]);
    return path;
}

int authenticate(int role, const char *username, const char *password, int *user_id) {
    FILE *fp = fopen(get_csv_path(role), "r");

    printf("DEBUG: Trying to open: %s\n", get_csv_path(role));

    if (!fp) {
        printf("DEBUG: Cannot open file!\n");
        return 0;
    }

    printf("DEBUG: File opened successfully\n");
    printf("DEBUG: Looking for user='%s' pass='%s'\n", username, password);

    char line[256];
    fgets(line, sizeof(line), fp); // Skip header

    while (fgets(line, sizeof(line), fp)) {
        int id;
        char user[50], pass[50];

        if (role == ROLE_CUSTOMER) {
            int active;
            if (sscanf(line, "%d,%49[^,],%49[^,],%d", &id, user, pass, &active) == 4) {
                printf("DEBUG: Read line - id=%d user='%s' pass='%s' active=%d\n", id, user, pass, active);

                if (strcmp(user, username) == 0 && strcmp(pass, password) == 0 && active) {
                    printf("DEBUG: Match found! Checking session...\n");
                    if (check_and_register_login(role, id)) {
                        *user_id = id;
                        fclose(fp);
                        printf("DEBUG: Login successful\n");
                        return 1;
                    } else {
                        printf("DEBUG: Already logged in\n");
                    }
                }
            }
        } else {
            if (sscanf(line, "%d,%49[^,],%49[^\n]", &id, user, pass) == 3) {
                if (strcmp(user, username) == 0 && strcmp(pass, password) == 0) {
                    if (check_and_register_login(role, id)) {
                        *user_id = id;
                        fclose(fp);
                        return 1;
                    }
                }
            }
        }
    }

    printf("DEBUG: No match found\n");
    fclose(fp);
    return 0;
}


// int authenticate(int role, const char *username, const char *password, int *user_id) {
//     FILE *fp = fopen(get_csv_path(role), "r");
//     if (!fp) return 0;
//
//     char line[256];
//     fgets(line, sizeof(line), fp);
//
//     while (fgets(line, sizeof(line), fp)) {
//         int id;
//         char user[50], pass[50];
//
//         if (role == ROLE_CUSTOMER) {
//             double balance;
//             int active;
//             if (sscanf(line, "%d,%49[^,],%49[^,],%lf,%d", &id, user, pass, &balance, &active) == 5) {
//                 if (strcmp(user, username) == 0 && strcmp(pass, password) == 0 && active) {
//                     if (check_and_register_login(role, id)) {
//                         *user_id = id;
//                         fclose(fp);
//                         return 1;
//                     }
//                 }
//             }
//         } else {
//             if (sscanf(line, "%d,%49[^,],%49[^\n]", &id, user, pass) == 3) {
//                 if (strcmp(user, username) == 0 && strcmp(pass, password) == 0) {
//                     if (check_and_register_login(role, id)) {
//                         *user_id = id;
//                         fclose(fp);
//                         return 1;
//                     }
//                 }
//             }
//         }
//     }
//
//     fclose(fp);
//     return 0;
// }

void logout_user(int role, int user_id) {
    register_logout(role, user_id);
}
