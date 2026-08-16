#include "session_manager.h"
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static SessionManager *sessions = NULL;
static int sem_session; // Keep only session semaphore

#if defined(__linux__)
union semun { int val; };
#endif

static void sem_lock(int sem_id) {
    struct sembuf op = {0, -1, SEM_UNDO};
    semop(sem_id, &op, 1);
}

static void sem_unlock(int sem_id) {
    struct sembuf op = {0, 1, SEM_UNDO};
    semop(sem_id, &op, 1);
}

void init_session_manager() {
    // Create shared memory segment for 100 sessions
    int shm_id = shmget(SHM_KEY_SESSIONS, sizeof(SessionManager), IPC_CREAT | 0666);

    // Attach shared memory to this process
    sessions = (SessionManager*)shmat(shm_id, NULL, 0);

    // Zero out all session data
    memset(sessions, 0, sizeof(SessionManager));

    // Create semaphore for protecting session access (binary semaphore)
    sem_session = semget(SHM_KEY_SESSIONS + 1, 1, IPC_CREAT | 0666);

    // Set semaphore value to 1 (unlocked state)
    union semun arg;
    arg.val = 1;
    semctl(sem_session, 0, SETVAL, arg);

    printf("✓ Session manager initialized\n");
}


int check_and_register_login(int role, int user_id) {
    sem_lock(sem_session);

    for (int i = 0; i < MAX_SESSIONS; i++) {
        if (sessions->sessions[i].active &&
            sessions->sessions[i].role == role &&
            sessions->sessions[i].user_id == user_id) {
            sem_unlock(sem_session);
            return 0;
        }
    }

    for (int i = 0; i < MAX_SESSIONS; i++) {
        if (!sessions->sessions[i].active) {
            sessions->sessions[i].role = role;
            sessions->sessions[i].user_id = user_id;
            sessions->sessions[i].active = 1;
            sem_unlock(sem_session);
            return 1;
        }
    }

    sem_unlock(sem_session);
    return 0;
}

void register_logout(int role, int user_id) {
    sem_lock(sem_session);
    for (int i = 0; i < MAX_SESSIONS; i++) {
        if (sessions->sessions[i].role == role &&
            sessions->sessions[i].user_id == user_id) {
            sessions->sessions[i].active = 0;
            break;
        }
    }
    sem_unlock(sem_session);
}
