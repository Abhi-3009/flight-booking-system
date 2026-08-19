#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>

void send_message(int sock, const char *msg) {
    printf("DEBUG: send_message called\n");
    write(sock, msg, strlen(msg));
}

void* handle_client(void *arg) {
    int sock = *(int*)arg;
    free(arg);
    printf("DEBUG: handle_client started for sock %d\n", sock);
    send_message(sock, "Hello from server\n");
    return NULL;
}

int main() {
    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(9090);
    bind(server_sock, (struct sockaddr*)&addr, sizeof(addr));
    listen(server_sock, 10);
    printf("Server listening\n");
    while (1) {
        int client_sock = accept(server_sock, NULL, NULL);
        printf("DEBUG: accept returned %d\n", client_sock);
        int *sock_ptr = malloc(sizeof(int));
        *sock_ptr = client_sock;
        pthread_t tid;
        pthread_create(&tid, NULL, handle_client, sock_ptr);
        pthread_detach(tid);
    }
}
