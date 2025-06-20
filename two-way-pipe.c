#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <unistd.h>
#include <sys/wait.h>
#define BUFSIZE 256
int sid_child;
int sid_parent;

void sem_wait(int sid) {
    struct sembuf sb;
    sb.sem_num = 0;
    sb.sem_op = -1;
    sb.sem_flg = 0;
    if (semop(sid, &sb, 1) == -1) {
        perror("semop wait");
        exit(1);
    }
}

void sem_signal(int sid) {
    struct sembuf sb;
    sb.sem_num = 0;
    sb.sem_op = 1;
    sb.sem_flg = 0;
    if (semop(sid, &sb, 1) == -1) {
        perror("semop signal");
        exit(1);
    }
}

int main(int argc, char *argv[]){
    char buf[BUFSIZE];
    int fdc2p[2]; // child to parent
    int fdp2c[2]; // parent to child
    int pid, msglen, status;
    key_t key_child, key_parent;

    // Create a unique key for the semaphore
    if ((key_child = ftok(".", 1)) == -1 || (key_parent = ftok(".", 2)) == -1) {
        fprintf(stderr, "ftok path does not exist.\n");
        exit(1);
    }
    // Create a semaphore set
    if ((sid_child = semget(key_child, 1, IPC_CREAT | 0666)) == -1 || (sid_parent = semget(key_parent, 1, IPC_CREAT | 0666)) == -1) {
        perror("semget failed.");
        exit(1);
    }
    // Initialize the semaphore to 0
    if (semctl(sid_child, 0, SETVAL, 0) == -1 || semctl(sid_parent, 0, SETVAL, 0) == -1) {
        perror("semctl failed.");
        exit(1);
    }

    if (argc != 3) {
        printf("bad argument.\n");
        exit(1);
    }

    if (pipe(fdc2p) == -1 || pipe(fdp2c) == -1) {
        perror("pipe failed.");
        exit(1);
    }
    
    if ((pid=fork())== -1) {
        perror("fork failed.");
        exit(1);
    }


    if (pid == 0) { // child process
        close(fdc2p[0]);
        close(fdp2c[1]);

        msglen = strlen(argv[1]) + 1;
        if (write(fdc2p[1], argv[1], msglen) == -1) {
            perror("pipe write.");
            exit(1);
        }
        close(fdc2p[1]);
        sem_signal(sid_parent); // parent can read

        sem_wait(sid_child); // wait for parent to send message

        if (read(fdp2c[0], buf, BUFSIZE) == -1) {
            perror("pipe read.");
            exit(1);
        }
        close(fdp2c[0]);
        printf("Message from parent process: \n\t%s\n", buf);

        exit(0);
    } else { // parent process
        close(fdc2p[1]);
        close(fdp2c[0]);

        msglen = strlen(argv[2]) + 1;
        if (write(fdp2c[1], argv[2], msglen) == -1) {
            perror("pipe write.");
            exit(1);
        }
        close(fdp2c[1]);
        sem_signal(sid_child); // child can read

        sem_wait(sid_parent); // wait for child to send message

        if (read(fdc2p[0], buf, BUFSIZE) == -1) {
            perror("pipe read.");
            exit(1);
        }
        close(fdc2p[0]);
        printf("Message from child process: \n\t%s\n", buf);

        wait(&status);
    }

    semctl(sid_child, 0, IPC_RMID);
    semctl(sid_parent, 0, IPC_RMID);

    return 0;
}