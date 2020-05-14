#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define callback_ip "127.1"
#define callback_port 4444

int main(){
    int sockfd;
    struct sockaddr_in c2addr;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        exit(1);
    }

    bzero(&c2addr, sizeof(c2addr));
    c2addr.sin_family = AF_INET;
    c2addr.sin_addr.s_addr = inet_addr(callback_ip);
    c2addr.sin_port = htons(callback_port); 

    if (connect(sockfd, (struct sockaddr*)&c2addr, sizeof(c2addr)) != 0){
        perror("failed to connect");
        exit(2);
    }

    close(sockfd);

    return 0;
}