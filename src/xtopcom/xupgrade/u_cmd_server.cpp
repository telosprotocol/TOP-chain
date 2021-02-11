#include "u_cmd_server.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <unistd.h>
#include "u_string.h"

#define MAX_FD_SETSIZE 64

void start_cmd_server(upgrade_config_t& config) {
    int i, maxi, maxfd, listenfd, connfd, sockfd;
    int nready, client[MAX_FD_SETSIZE];
    ssize_t n;
    fd_set rset, allset;
    char buf[8096];
    socklen_t cliaddr_len;
    struct sockaddr_in
    cliaddr, servaddr;
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(config.cmd_port);
    if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1) {
		printf("bind error\n");
		return;
    }
    if (listen(listenfd, 10) == -1) {
		printf("listen error\n");
		return;
    }
    maxfd = listenfd;
    maxi = -1;
    for (i = 0; i < MAX_FD_SETSIZE; i++)
        client[i] = -1; /* -1 indicates available entry */
    FD_ZERO(&allset);
    FD_SET(listenfd, &allset);
    for ( ; ; ) {
        rset = allset; /* structure assignment */
        nready = select(maxfd+1, &rset, NULL, NULL, NULL);
        if (nready < 0) {
            printf("select error\n");
			return;
        }
        if (FD_ISSET(listenfd, &rset)) { /* new client connection */
            cliaddr_len = sizeof(cliaddr);
            connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &cliaddr_len);
			if (connfd == -1) {
				printf("accept error\n");
				continue;
			}
			char str[128];
            printf("received from %s at PORT %d\n", inet_ntop(AF_INET, &cliaddr.sin_addr, str, sizeof(str)), ntohs(cliaddr.sin_port));
            for (i = 0; i < MAX_FD_SETSIZE; i++) {
                if (client[i] < 0) {
                    client[i] = connfd; /* save descriptor */
                    break;
                }
            }
            if (i == MAX_FD_SETSIZE) {
                fputs("too many clients\n", stderr);
                return;
            }
            FD_SET(connfd, &allset); /* add new descriptor to set */
            if (connfd > maxfd)
                maxfd = connfd; /* for select */
            if (i > maxi)
                maxi = i; /* max index in client[] array */
            if (--nready == 0)
                continue; /* no more readable descriptors */
        }
        for (i = 0; i <= maxi; i++) {
            /* check all clients 714 for data */
            if ( (sockfd = client[i]) < 0)
                continue;
            if (FD_ISSET(sockfd, &rset)) {
                if ( (n = recv(sockfd, buf, sizeof(buf), 0)) == 0) {
                    close(sockfd);
                    FD_CLR(sockfd, &allset);
                    client[i] = -1;
					printf("close socket%d\n", i);
                } else {
//                    int j;
//                    for (j = 0; j < n; j++)
//                        buf[j] = toupper(buf[j]);
					printf("cmd server recv:%s\n", u_string::HexEncode(std::string(buf, n)).c_str());
//                    Write(sockfd, buf, n);
                }
                if (--nready == 0)
                    break; /* no more readable descriptors */
            }
        }
    }
	return;
}
