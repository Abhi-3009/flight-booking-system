#include "session_manager.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static SessionManager sessions_data;
static SessionManager *sessions = &sessions_data;
static pthread_mutex_t session_mutex = PTHREAD_MUTEX_INITIALIZER;

void init_session_manager() {
    pthread_mutex_lock(&session_mutex);
    memset(sessions, 0, sizeof(SessionManager));
    pthread_mutex_unlock(&session_mutex);
    printf("✓ Session manager initialized\n");
}

int check_and_register_login(int role, int user_id) {
    pthread_mutex_lock(&session_mutex);

    // Check if user is already logged in
    for (int i = 0; i < MAX_SESSIONS; i++) {
        if (sessions->sessions[i].active &&
            sessions->sessions[i].role == role &&
            sessions->sessions[i].user_id == user_id) {
            pthread_mutex_unlock(&session_mutex);
            return 0;
        }
    }

    // Register active session
    for (int i = 0; i < MAX_SESSIONS; i++) {
        if (!sessions->sessions[i].active) {
            sessions->sessions[i].role = role;
            sessions->sessions[i].user_id = user_id;
            sessions->sessions[i].active = 1;
            pthread_mutex_unlock(&session_mutex);
            return 1;
        }
    }

    pthread_mutex_unlock(&session_mutex);
    return 0; // Session table full
}

void register_logout(int role, int user_id) {
    pthread_mutex_lock(&session_mutex);
    for (int i = 0; i < MAX_SESSIONS; i++) {
        if (sessions->sessions[i].role == role &&
            sessions->sessions[i].user_id == user_id) {
            sessions->sessions[i].active = 0;
            sessions->sessions[i].role = 0;
            sessions->sessions[i].user_id = 0;
            break;
        }
    }
    pthread_mutex_unlock(&session_mutex);
}
