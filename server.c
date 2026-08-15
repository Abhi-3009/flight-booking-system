#include "common.h"
#include "auth.h"
#include "customer.h"
#include "employee.h"
#include "manager.h"
#include "admin.h"
#include "utils.h"
#include "session_manager.h"
#include <pthread.h>
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

    while (1) {
        send_message(sock,
            "\n=== Airline Booking System ===\n"
            "1. Passenger\n"
            "2. Agent\n"
            "3. Manager\n"
            "4. Admin\n"
            "5. Exit\n"
            "Select role: ");

        if (!read_input(sock, buffer, BUFFER_SIZE)) break;
        int role = atoi(buffer);

        if (role == 5) {
            send_message(sock, "Goodbye!\n");
            break;
        }

        if (role < 1 || role > 4) {
            send_message(sock, "Invalid role!\n");
            continue;
        }

        send_message(sock, "Username: ");
        if (!read_input(sock, buffer, BUFFER_SIZE)) break;
        char username[50];
        strcpy(username, buffer);

        send_message(sock, "Password: ");
        if (!read_input(sock, buffer, BUFFER_SIZE)) break;
        char password[50];
        strcpy(password, buffer);

        int user_id;
        if (authenticate(role, username, password, &user_id)) {
            send_message(sock, "\n✓ Login successful!\n");

            // Use switch statement instead of function pointers
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

            logout_user(role, user_id);
            send_message(sock, "\nLogged out.\n");
        } else {
            send_message(sock, "\n✗ Authentication failed!\n");
        }
    }

    close(sock);
    return NULL;
}

int main() {
    mkdir(DATA_DIR, 0755);
    init_session_manager();

    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    bind(server_sock, (struct sockaddr*)&addr, sizeof(addr));
    listen(server_sock, 10);

    printf("Server listening on port %d\n", PORT);

    while (1) {
        int client_sock = accept(server_sock, NULL, NULL);
        int *sock_ptr = malloc(sizeof(int));
        *sock_ptr = client_sock;

        pthread_t tid;
        pthread_create(&tid, NULL, handle_client, sock_ptr);
        pthread_detach(tid);
    }

    return 0;
}
