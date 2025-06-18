#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <unistd.h>
#include <sys/wait.h>
#define NUMPROCS 4
char filename[]="counter";
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

int count1() {
    FILE *ct;
    int count;

    if ((ct=fopen(filename, "r"))==NULL) exit(1);
    fscanf(ct, "%d\n", &count);
    count++;
    fclose(ct);

    if ((ct=fopen(filename, "w"))==NULL) exit(1);
    fprintf(ct, "%d\n", count);
    fclose(ct);

    return count;
}

int main() {
    int i, count, pid, status;
    FILE *ct;
    key_t key;
    setbuf(stdout, NULL);

    // Create a unique key for the semaphore
    if ((key = ftok(".", 1)) == -1) {
        fprintf(stderr, "ftok path does not exist.\n");
        exit(1);
    }
    // Create a semaphore set
    if ((sid = semget(key, 1, IPC_CREAT | 0666)) == -1) {
        perror("semget failed.");
        exit(1);
    }
    // Initialize the semaphore to 1
    if (semctl(sid, 0, SETVAL, 1) == -1) {
        perror("semctl failed.");
        exit(1);
    }

    count = 0;
    // Initialize the counter file
    if ((ct=fopen(filename, "w"))==NULL) exit(1);
    fprintf(ct, "%d\n", count); // Write initial count to file
    fclose(ct);
    

    for (i=0; i<NUMPROCS; i++) {
        if ((pid=fork()) == -1) {
            perror("fork failed.");
            exit(1);
        }
        if (pid == 0) {
            // add semaphore
            sem_wait();
            count = count1();
            printf("count = %d\n", count);
            sem_signal();
            exit(0);
        }
    }
    for (i=0; i<NUMPROCS; i++) {
        wait(&status);
    }
    if (semctl(sid, 0, IPC_RMID, 0) == -1) {
        perror("semctl remove failed.");
        exit(1);
    }
    exit(0);
}