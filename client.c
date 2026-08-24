#include "common.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int main() {
  int sock = socket(AF_INET, SOCK_STREAM, 0);

  struct sockaddr_in addr = {0};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(PORT);
  inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

  if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    printf("Connection failed\n");
    return 1;
  }

  printf("Connected to server\n\n");
  setvbuf(stdout, NULL, _IONBF, 0);

  char buffer[BUFFER_SIZE];
  char input[BUFFER_SIZE];
  struct pollfd fds[2];

  fds[0].fd = sock;
  fds[0].events = POLLIN;
  fds[1].fd = STDIN_FILENO;
  fds[1].events = POLLIN;

  while (1) {
    if (poll(fds, 2, 100) > 0) {
      if (fds[0].revents & POLLIN) {
        memset(buffer, 0, BUFFER_SIZE);
        int n = read(sock, buffer, BUFFER_SIZE - 1);
        if (n <= 0)
          break;
        printf("%s", buffer);
        fflush(stdout);
      }

      if (fds[1].revents & POLLIN) {
        if (!fgets(input, BUFFER_SIZE, stdin))
          break;
        int len = strlen(input);
        if (len == 0) {
          input[0] = '\n';
          input[1] = '\0';
          len = 1;
        }
        if (write(sock, input, len) <= 0)
          break;
      }
    }
  }

  close(sock);
  printf("\nDisconnected\n");
  return 0;
}
