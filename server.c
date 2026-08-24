#include "common.h"
#include "auth.h"
#include "customer.h"
#include "employee.h"
#include "manager.h"
#include "admin.h"
#include "utils.h"
#include "session_manager.h"
#include "seat_manager.h"
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>

void* handle_client(void *arg) {
    int sock = *(int*)arg;
    free(arg);

    char buffer[BUFFER_SIZE];
    int active_role = 0;
    int active_user_id = 0;

    while (1) {
        send_message(sock,
            "\n=================================\n"
            "   Airline Booking System        \n"
            "=================================\n"
            "1. Passenger\n"
            "2. Agent\n"
            "3. Manager\n"
            "4. Admin\n"
            "5. Exit\n"
            "Select role: ");

        if (!read_input(sock, buffer, BUFFER_SIZE)) break;
        if (!is_numeric(buffer)) {
            send_message(sock, "Invalid choice! Please enter a number.\n");
            continue;
        }
        int role = atoi(buffer);

        if (role == 5) {
            send_message(sock, "Goodbye!\n");
            break;
        }

        if (role < 1 || role > 4) {
            send_message(sock, "Invalid role! Please choose 1-5.\n");
            continue;
        }

        send_message(sock, "Username: ");
        if (!read_input(sock, buffer, BUFFER_SIZE)) break;
        char username[50];
        strncpy(username, buffer, sizeof(username) - 1);
        username[sizeof(username) - 1] = '\0';

        send_message(sock, "Password: ");
        if (!read_input(sock, buffer, BUFFER_SIZE)) break;
        char password[50];
        strncpy(password, buffer, sizeof(password) - 1);
        password[sizeof(password) - 1] = '\0';

        int user_id = 0;
        int auth_res = authenticate(role, username, password, &user_id);
        if (auth_res == 1) {
            active_role = role;
            active_user_id = user_id;
            send_message(sock, "\n✓ Login successful!\n");

            switch (role) {
                case ROLE_CUSTOMER:
                    handle_customer(sock, user_id);
                    break;
                case ROLE_EMPLOYEE:
                    handle_employee(sock, user_id);
                    break;
                case ROLE_MANAGER:
                    handle_manager(sock, user_id);
                    break;
                case ROLE_ADMIN:
                    handle_admin(sock, user_id);
                    break;
            }

            if (role == ROLE_CUSTOMER) {
                release_customer_holds(user_id);
            }
            logout_user(role, user_id);
            active_role = 0;
            active_user_id = 0;
            send_message(sock, "\nLogged out.\n");
        } else if (auth_res == -1) {
            send_message(sock, "\n✗ Account is already logged in from another session!\n");
        } else if (auth_res == -2) {
            send_message(sock, "\n✗ Account is deactivated. Please contact manager.\n");
        } else {
            send_message(sock, "\n✗ Authentication failed! Invalid username or password.\n");
        }
    }

    if (active_role != 0 && active_user_id != 0) {
        if (active_role == ROLE_CUSTOMER) {
            release_customer_holds(active_user_id);
        }
        logout_user(active_role, active_user_id);
    }

    close(sock);
    return NULL;
}

int main() {
    signal(SIGPIPE, SIG_IGN);
    mkdir(DATA_DIR, 0755);
    init_session_manager();
    init_seat_manager();

    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    if (bind(server_sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Bind failed");
        return 1;
    }
    listen(server_sock, 10);

    printf("==========================================\n");
    printf("Flight Booking Server started on port %d\n", PORT);
    printf("==========================================\n");

    while (1) {
        int client_sock = accept(server_sock, NULL, NULL);
        if (client_sock < 0) continue;

        int *sock_ptr = malloc(sizeof(int));
        *sock_ptr = client_sock;

        pthread_t tid;
        pthread_create(&tid, NULL, handle_client, sock_ptr);
        pthread_detach(tid);
    }

    return 0;
}
