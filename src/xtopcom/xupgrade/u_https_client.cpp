// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <stdio.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <string>
#include "u_https_client.h"
using namespace std;

SSL *ssl;
int sock;

int RecvPacket(std::string& return_https)
{
    int len;
    char buf[8096];
    int nContentLength = -1;

    while(1){
        len=SSL_read(ssl, buf, sizeof(buf));
        if (len <= 0)
            break;
        return_https += std::string(buf, len);

        std::string::size_type found = return_https.find("Content-Length: ");
        if (found != std::string::npos){
            std::string strLen = return_https.substr(found + 16); // + char(0);
            found = strLen.find("\r\n");
            if (found == std::string::npos){
                printf("cannot find end of Content-Length.\n");
                return 0;
            }
            strLen[found] = 0;
            nContentLength = std::stoi(strLen);
        }
        found = return_https.find("\r\n\r\n");
        if (nContentLength != -1 && found != std::string::npos){
            found += 4;
            if (return_https.size() >= found + nContentLength) {  // recv all
                printf("https size:%d,found:%d, content length:%d\n", (int)return_https.size(), (int)found , nContentLength);
                return 2;
            }
        }
        
        int pending;
        pending = SSL_pending(ssl);
        if(pending <= 0) {
            return 0;
        }
    }

    if (len < 0) {
        int err = SSL_get_error(ssl, len);
        if (err == SSL_ERROR_WANT_READ)
            return 1;
        if (err == SSL_ERROR_WANT_WRITE)
            return 1;
        if (err == SSL_ERROR_ZERO_RETURN || err == SSL_ERROR_SYSCALL || err == SSL_ERROR_SSL)
            return 1;
    }

    return 3;
}

int SendPacket(const char *buf)
{
    int len = SSL_write(ssl, buf, strlen(buf));
    if (len < 0) {
        int err = SSL_get_error(ssl, len);
        switch (err) {
        case SSL_ERROR_WANT_WRITE:
            return 0;
        case SSL_ERROR_WANT_READ:
            return 0;
        case SSL_ERROR_ZERO_RETURN:
        case SSL_ERROR_SYSCALL:
        case SSL_ERROR_SSL:
        default:
            return -1;
        }
    }
    return 0;
}

void log_ssl()
{
    int err;
    while ((err = ERR_get_error()) != 0) {
        char *str = ERR_error_string(err, 0);
        if (!str)
            return;
        printf(str);
        printf("\n");
        fflush(stdout);
    }
}

int get_https(const std::string& https_addr, std::string& return_https) {
    std::string strtemp;
    std::string strHost;
    std::string strtemp2;
    std::string::size_type pos = https_addr.find("//");
    strtemp = https_addr.substr(pos + 2);
    pos = strtemp.find("/");
    strtemp2 = strtemp.substr(pos);
    strtemp = strtemp.substr(0, pos);
    strHost = strtemp;
    strtemp += char(0);
    printf("domain:%s\n", strtemp.c_str());
    printf("web addr:%s\n", strtemp2.c_str());
    return_https.clear();

    struct hostent *host;
    struct in_addr in;
    struct sockaddr_in addr_in;
    if((host=gethostbyname(strtemp.c_str())) == NULL) {
        printf("gethostbyname error.\n");
        return 1;
    }
    memcpy(&addr_in.sin_addr.s_addr,host->h_addr,4);
    in.s_addr=addr_in.sin_addr.s_addr;
    printf("IP : %s \n",inet_ntoa(in));
    strtemp = inet_ntoa(in);

    int s;
    s = socket(AF_INET, SOCK_STREAM, 0);
    if (!s) {
        printf("Error creating socket.\n");
        return 1;
    }
    struct sockaddr_in sa;
    memset (&sa, 0, sizeof(sa));
    sa.sin_family      = AF_INET;
    sa.sin_addr.s_addr = inet_addr(strtemp.c_str());
    sa.sin_port        = htons (443);
    socklen_t socklen = sizeof(sa);
    if (connect(s, (struct sockaddr *)&sa, socklen)) {
        printf("Error connecting to server.\n");
        return 1;
    }
    SSL_library_init();
    SSLeay_add_ssl_algorithms();
    SSL_load_error_strings();
    const SSL_METHOD *meth = TLSv1_2_client_method();
    SSL_CTX *ctx = SSL_CTX_new (meth);
    ssl = SSL_new (ctx);
    if (!ssl) {
        printf("Error creating SSL.\n");
        log_ssl();
        return 1;
    }
    sock = SSL_get_fd(ssl);
    SSL_set_fd(ssl, s);
    int err = SSL_connect(ssl);
    if (err <= 0) {
        printf("Error creating SSL connection.  err=%x\n", err);
        log_ssl();
        fflush(stdout);
        return 1;
    }
    printf ("SSL connection using %s\n", SSL_get_cipher (ssl));

    std::string request;
    request += "GET " + strtemp2 + " HTTP/1.1\r\n";
    request += std::string("Host: ") + strHost + "\r\n";
    request += "User-Agent: topnetwork.org\r\n";
    request += "\r\n";
    printf("https request:%s\n", request.c_str());
    SendPacket(request.c_str());

    fd_set fds;
    struct timeval timeout={5,0};
    int nRet;
    while(1) {
        FD_ZERO(&fds);
        FD_SET(s,&fds);
        nRet = select(s + 1, &fds, NULL, NULL, &timeout);
        if (nRet == 0)  {
            printf("timeout.\n");
            std::string::size_type found = return_https.find("\r\n\r\n");
            if (found == std::string::npos){
                printf("found error.\n");
                return 1;
            }
            std::string header;
            header = return_https.substr(0, found);
            printf("header:%s\n", header.c_str());
            return_https = return_https.substr(found + 4); // + char(0);

            SSL_shutdown(ssl);
            SSL_free(ssl);
            close(s);
            SSL_CTX_free(ctx);
            return 0;
        }
        if (nRet == -1){
            printf("recv error.\n");
            return 1;
        }
        if(FD_ISSET(s,&fds)) {
            int recv_ret = RecvPacket(return_https);
//          printf("recv_ret:%d\n", recv_ret);
            if ( recv_ret == 0)
                continue;
            
            std::string::size_type found = return_https.find("\r\n\r\n");
            if (found == std::string::npos){
                printf("found error.\n");
                continue;
            }
            std::string header;
            header = return_https.substr(0, found);
            printf("header:%s\n", header.c_str());
            return_https = return_https.substr(found + 4); // + char(0);

            SSL_shutdown(ssl);
            SSL_free(ssl);
            close(s);
            SSL_CTX_free(ctx);
            return 0;

        }
    }

    return 0;
}

