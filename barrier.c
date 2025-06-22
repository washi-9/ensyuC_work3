#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<sys/ipc.h>
#include<sys/sem.h>
#include<sys/shm.h>
#include<sys/wait.h>
#include<unistd.h>

#define NUM_PROCS 5

void sem_op(int sid, int sem_num, int op) {
    struct sembuf sb;
    sb.sem_num = sem_num;
    sb.sem_op = op;
    sb.sem_flg = 0;
    if (semop(sid, &sb, 1) == -1) {
        perror("semop failed");
        exit(1);
    }
}

int main() {
    int i, status, sid, shmid;
    key_t key;
    int *counter;

    if ((key = ftok(".", 1)) == -1) {
        fprintf(stderr, "ftok path does not exist.\n");
        exit(1);
    }

    if ((shmid = shmget(key, sizeof(int), 0666 | IPC_CREAT)) == -1) {
        perror("shmget failed");
        exit(1);
    }

    if ((counter = (int *)shmat(shmid, 0, 0)) == (int *)-1) {
        perror("shmat failed");
        exit(1);
    }
    *counter = 0;

    if ((sid = semget(key, 2, 0666 | IPC_CREAT)) == -1) {
        perror("semget failed");
        exit(1);
    }

    // semaphore
    if (semctl(sid, 0, SETVAL, 1) == -1) {
        perror("semctl failed");
        exit(1);
    }

    // barrier
    if (semctl(sid, 1, SETVAL, 0) == -1) {
        perror("semctl failed");
        exit(1);
    }

    setbuf(stdout, NULL);

    for (i = 0; i < NUM_PROCS; i++) {
        if (fork() == 0) {
            srand(getpid());

            printf("child process %d started. \n", getpid());
            sleep(rand() % NUM_PROCS + 1);
            printf("child process %d finished. \n", getpid());

            // barrier
            sem_op(sid, 0, -1); 
            (*counter)++;
            sem_op(sid, 0, 1); 

            if (*counter == NUM_PROCS) {
                printf("I'm the last process %d\n", getpid());
                for (int j = 0; j < NUM_PROCS - 1; j++) {
                    sem_op(sid, 1, 1);
                }
            } else {
                printf("I'm waiting for other processes %d\n", getpid());
                sem_op(sid, 1, -1); // wait for the barrier
            }
        printf("child process %d started. \n", getpid());
        exit(0);
        } 
    }

    for (i = 0; i < NUM_PROCS; i++) {
        wait(&status);
    }

    shmdt(counter);
    shmctl(shmid, IPC_RMID, 0);
    semctl(sid, 0, IPC_RMID, 0);

    return 0;
}