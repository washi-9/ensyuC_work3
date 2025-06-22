#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <unistd.h>
#include <sys/wait.h>
int sid;

void sem_wait(int num) {
    struct sembuf sb;
    sb.sem_num = num;
    sb.sem_op = -1;
    sb.sem_flg = 0;
    if (semop(sid, &sb, 1) == -1) {
        perror("semop wait");
        exit(1);
    }
}

void sem_signal(int num) {
    struct sembuf sb;
    sb.sem_num = num;
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

    if ((key = ftok(".", 0)) == -1) {
        fprintf(stderr, "ftok path does not exist.\n");
        exit(1);
    }

    if ((sid = semget(key, 2, IPC_CREAT | 0666)) == -1) {
        perror("semget failed.");
        exit(1);
    }

    if (semctl(sid, 0, SETVAL, 1) == -1) {
        perror("semctl failed.");
        exit(1);
    }

    if (semctl(sid, 1, SETVAL, 0) == -1) {
        perror("semctl failed.");
        exit(1);
    }

    int A[10],B[10];
    for (i = 0; i < 10; i++) {
        A[i] = i + 1;
    }
    for (i = 0; i < 10; i++) {
        B[i] = i + 11;
    }
    if ((pid = fork()) == -1) {
        perror("fork failed.");
        exit(1);
    }
    if (pid == 0) {
        for (i = 0; i < 10; i++) {
            sem_wait(0);
            printf("Child: %d\n", A[i]);
            sem_signal(1);
        }
        exit(0);
    } else {
        for (i = 0; i < 10; i++) {
            sem_wait(1);
            printf("Parent: %d\n", B[i]);
            sem_signal(0);
        }
        wait(&status);
    }

    if (semctl(sid, 0, IPC_RMID) == -1) {
        perror("semctl IPC_RMID failed.");
        exit(1);
    }
    return 0;
}