#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <unistd.h>
#include <sys/wait.h>
int sid;

void sem_wait() {
    struct sembuf sb;
    sb.sem_num = 0;
    sb.sem_op = -1;
    sb.sem_flg = 0;
    if (semop(sid, &sb, 1) == -1) {
        perror("semop wait");
        exit(1);
    }
}

void sem_signal() {
    struct sembuf sb;
    sb.sem_num = 0;
    sb.sem_op = 1;
    sb.sem_flg = 0;
    if (semop(sid, &sb, 1) == -1) {
        perror("semop signal");
        exit(1);
    }
}

int main() {
    int i, count, pid, status;
    key_t key;
    setbuf(stdout, NULL);

    if ((key = ftok(".", 1)) == -1) {
        fprintf(stderr, "ftok path does not exist.\n");
        exit(1);
    }

    if ((sid = semget(key, 1, IPC_CREAT | 0666)) == -1) {
        perror("semget failed.");
        exit(1);
    }

    if (semctl(sid, 0, SETVAL, 1) == -1) {
        perror("semctl failed.");
        exit(1);
    }

    int A[100],B[100];
    for (i = 0; i < 100; i++) {
        A[i] = i + 1;
    }
    for (i = 100; i < 200; i++) {
        B[i - 100] = i + 1;
    }
    if ((pid = fork()) == -1) {
        perror("fork failed.");
        exit(1);
    }
    if (pid == 0) {
        for (i = 0; i < 100; i++) {
            sem_wait();
            printf("Child: %d\n", A[i]);
            sem_signal();
        }
        exit(0);
    } else {
        for (i = 0; i < 100; i++) {
            sem_wait();
            printf("Parent: %d\n", B[i]);
            sem_signal();
        }
        wait(&status);
    }

    if (semctl(sid, 0, IPC_RMID) == -1) {
        perror("semctl IPC_RMID failed.");
        exit(1);
    }
    return 0;
}