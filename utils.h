#ifndef UTILS_H
#define UTILS_H

void send_message(int sock, const char *msg);
int read_input(int sock, char *buffer, int size);
int get_next_id_csv(const char *csv_path);
int lock_file(int fd, int lock_type);
int unlock_file(int fd);
void trim_string(char *str);
int is_numeric(const char *str);
int is_alpha(const char *str);

#endif
