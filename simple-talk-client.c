#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<netdb.h>
#include<unistd.h>
#include<string.h>
#include<stdio.h>
#include<stdlib.h>
#include<signal.h>
#include<sys/select.h>
#include<stdbool.h>

#define PORT 10130
#define BUFFER_SIZE 1024
#define TIMEOUT 10

bool is_timed_out = false;
bool is_ctrl_c = false;

void myalarm(int sec) {
    static pid_t timer_pid = 0;
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
    is_timed_out = true;
}

void ctrlC() {
    is_ctrl_c = true;
}

int main(int argc, char **argv) {
    int sock;
    struct sockaddr_in echoServAddr;
    struct hostent *hp;
    char *servIP;
    char rbuf[BUFFER_SIZE];
    int bytesRcvd;
    struct timeval tv;
    fd_set rfds;

    if(signal(SIGALRM,timeout) == SIG_ERR) {
        perror("signal failed.");
        exit(1);
    }

    if(signal(SIGINT, ctrlC) == SIG_ERR) {
        perror("signal failed.");
        exit(1);
    }

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <host>\n", argv[0]);
        exit(1);
    }

    servIP = argv[1]; // Server IP address

    if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        perror("socket() failed");
        exit(1);
    }

    if ((hp = gethostbyname(servIP)) == NULL) {
        perror("gethostbyname() failed");
        exit(1);
    }

    memset(&echoServAddr, 0, sizeof(echoServAddr));
    echoServAddr.sin_family = AF_INET;
    memcpy(&echoServAddr.sin_addr, hp->h_addr, hp->h_length);
    echoServAddr.sin_port = htons(PORT); // default port

    if (connect(sock, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr)) < 0) {
        perror("connect() failed");
        close(sock);
        exit(1);
    }

    printf("connected\n");
    myalarm(TIMEOUT);

    while (!is_timed_out && !is_ctrl_c) {
        FD_ZERO(&rfds);
        FD_SET(0, &rfds);
        FD_SET(sock, &rfds);

        int maxfd = sock > 0 ? sock : 0;

        tv.tv_sec = 1;
        tv.tv_usec = 0;

        if (select(maxfd + 1, &rfds, NULL, NULL, &tv) > 0) {
            if (FD_ISSET(0, &rfds)) {
                // Read from stdin
                if (fgets(rbuf, BUFFER_SIZE, stdin) != NULL) {
                    myalarm(TIMEOUT);
                    size_t len = strlen(rbuf);
                    if (send(sock, rbuf, len, 0) != len) {
                        perror("send() failed");
                        break;
                    }
                }
            }

            if (FD_ISSET(sock, &rfds)) {
                // Read from socket
                bytesRcvd = recv(sock, rbuf, BUFFER_SIZE, 0);
                if (bytesRcvd < 0) {
                    perror("recv() failed");
                    break;
                } else if (bytesRcvd == 0) {
                    printf("Server closed connection\n");
                    break;
                } else {
                    rbuf[bytesRcvd] = '\0'; // Null-terminate the string
                    printf("> %s", rbuf);
                    myalarm(TIMEOUT);
                }
            }
        }
    }
    close(sock);
    if (is_timed_out) {
        printf("Connection timed out.\n");
    } else if (is_ctrl_c) {
        printf("\nConnection closed by user.\n");
    } else {
        printf("Connection closed by server.\n");
    }
    return 0;
}