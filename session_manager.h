#ifndef SESSION_MANAGER_H
#define SESSION_MANAGER_H

#include "common.h"

void init_session_manager();
int check_and_register_login(int role, int user_id);
void register_logout(int role, int user_id);

#endif
