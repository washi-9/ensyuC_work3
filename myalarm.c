#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#define BUFSIZE 256
#define TIMEOUT 10

static pid_t timer_pid = 0;

void myalarm(int sec) {
    if (timer_pid > 0) {
        kill(timer_pid, SIGTERM);
    }
    int pid;

    if ((pid = fork()) == -1) {
        perror("fork failed.");
        exit(1);
    }
    if (pid == 0) {
        sleep(sec);
        kill(getppid(), SIGALRM);
        exit(0);
    } else {
        if (signal(SIGCHLD, SIG_IGN) == SIG_ERR) {
        perror("signal failed.");
        exit(1);
        }
        timer_pid = pid;
    }
}

void timeout() {
    printf("This program is timeout.\n");
    exit(0);
}
int main() {
    char buf[BUFSIZE];
    if(signal(SIGALRM,timeout) == SIG_ERR) {
        perror("signal failed.");
        exit(1);
    }
    myalarm(TIMEOUT);
    while (fgets(buf, BUFSIZE, stdin) != NULL) {
        printf("echo: %s",buf);
        myalarm(TIMEOUT);
    }
}