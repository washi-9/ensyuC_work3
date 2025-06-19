#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define PORT 10130 // Default port number
#define BUFFER_SIZE 1024

int main(int argc, char **argv) {
    int sock, csock;
    fd_set rfds;
    struct timeval tv;
    struct sockaddr_in svr;
    struct sockaddr_in clt;
    int clen,bytesRcvd,reuse;
    char rbuf[BUFFER_SIZE];

    /* ソケットの生成 */
    if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        perror("socket");
        exit(1);
    }
    /* ソケットアドレス再利用の指定 */
    reuse = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        perror("setsockopt");
        close(sock);
        exit(1);
    }

    /* client 受付用ソケットの情報設定 */
    bzero(&svr, sizeof(svr));
    svr.sin_family = AF_INET;
    svr.sin_addr.s_addr = htonl(INADDR_ANY); /* 受付側の IP アドレスは任意 */
    svr.sin_port = htons(PORT); /* ポート番号 10130 を介して受け付ける */

    /* ソケットにソケットアドレスを割り当てる */
    if (bind(sock, (struct sockaddr *)&svr, sizeof(svr)) < 0) {
        perror("bind");
        exit(1);
    }
    /* 待ち受けクライアント数の設定 */
    if (listen(sock, 5) < 0) { /* 待ち受け数に 5 を指定 */
        perror("listen");
        exit(1);
    }

    clen = sizeof(clt);
    csock = accept(sock, (struct sockaddr *)&clt, &clen);
    if (csock < 0) {
        perror("accept");
        exit(1);
    }

    printf("connected\n");

    while(1) {
        FD_ZERO(&rfds);
        FD_SET(0,&rfds);
        FD_SET(csock,&rfds);

        int maxfd = csock > 0 ? csock : 0; // 最大のファイルディスクリプタを取得
        /* 監視する時間 */
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        /* 標準入力とソケットからの受信を同時に監視する */
        if (select(maxfd + 1,&rfds,NULL,NULL,&tv)>0) {
            if (FD_ISSET(0,&rfds)){
                // Read from stdin
                if (fgets(rbuf, BUFFER_SIZE, stdin) != NULL) {
                    size_t len = strlen(rbuf);
                    if (send(csock, rbuf, len, 0) != len) {
                        perror("send() failed");
                        break;
                    }
                }
            }

            if (FD_ISSET(csock,&rfds)) {
                // Read from socket
                bytesRcvd = recv(csock, rbuf, BUFFER_SIZE, 0);
                if (bytesRcvd < 0) {
                    perror("recv() failed");
                    break;
                } else if (bytesRcvd == 0) {
                    printf("Client disconnected\n");
                    break;
                } else {
                    rbuf[bytesRcvd] = '\0'; // Null-terminate the string
                    printf("> %s", rbuf);
                }
            }
        }
    }

    close(csock);
    close(sock);
    return 0;
}