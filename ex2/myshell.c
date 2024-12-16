// Since we're so special, we define macros for STDIN and STDOUT
#define STDOUT 1
#define STDIN 0
#include "shell.c"
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int specialChar, pipeIndex;

void SIGCHLD_handler(int signo)
{
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

// Check for special characters in the arglist, and set flags accordingly
void checkSpecials(int count, char** arglist) 
{
    specialChar = 0;
    if (strcmp(arglist[count - 1], "&") == 0)
    {
        specialChar = 1;
    }
    if (strcmp(arglist[count - 2], "<") == 0)
    {
        specialChar = 2;
    }
    if (strcmp(arglist[count - 2], ">") == 0)
    {
        specialChar = 3;
    }
    for (int i = 0; i < count; i++)
    {
        if (strcmp(arglist[i], "|") == 0)
            specialChar = 4;
            pipeIndex = i;
    }

}

int prepare(void)
{
    return 0;
}

int process_arglist(int count, char** arglist)
{
    signal(SIGINT, SIG_IGN);

    checkSpecials(count, arglist);

    switch (specialChar)
    {
    case 1: // Background
        pid_t pid;
        arglist[count - 1] = NULL; // Replace the & character with NULL for execvp
        pid = fork();
        switch (pid)
        {
        case 0:
            execvp(arglist[0], arglist);
            fprintf(stderr, "execvp failed while running the command, error: %s\n", strerror(errno));
            exit(1);
            break;
        
        case -1:
            fprintf(stderr, "fork failed, error: %s\n", strerror(errno));
            exit(1);
            break;

        default:
            break;
        }
        break;
    
    case 2: // Input redirection
        pid_t pid_child;
        int fd_in;
        arglist[count - 2] = NULL; // Replace the < character with NULL for execvp
        pid_child = fork();

        if (pid_child == 0)
        {
            fd_in = open(arglist[count - 1], O_RDONLY);
            if (fd_in == -1)
            {
                fprintf(stderr, "open failed while opening the file, error: %s\n", strerror(errno));
                exit(1);
            }
            dup2(fd_in, STDIN);
            close(fd_in); // Redirect file to stdin and close the file descriptor
            execvp(arglist[0], arglist); // Execute the command

            fprintf(stderr, "execvp failed while running the command, error: %s\n", strerror(errno)); // Error handling if execvp fails
            exit(1);
        }
        else
        {
            if (pid_child == -1)
            {
                fprintf(stderr, "fork failed, error: %s\n", strerror(errno));
                exit(1);
            }
            waitpid(pid_child, NULL, 0);
        }
        
        break;

    case 3: // Output redirection
        pid_t pid_child;
        int fd_out;
        arglist[count - 2] = NULL; // Replace the > character with NULL for execvp
        pid_child = fork();

        if (pid_child == 0)
        {
            fd_out = open(arglist[count - 1], O_WRONLY | O_CREAT | O_TRUNC, 0600);
            if (fd_out == -1)
            {
                fprintf(stderr, "open failed while opening the file, error: %s\n", strerror(errno));
                exit(1);
            }
            dup2(fd_out, STDOUT);
            close(fd_out); // Redirect stdout to file and close the file descriptor
            execvp(arglist[0], arglist); // Execute the command

            fprintf(stderr, "execvp failed while running the command, error: %s\n", strerror(errno)); // Error handling if execvp fails
            exit(1);
        }
        else
        {
            if (pid_child == -1)
            {
                fprintf(stderr, "fork failed, error: %s\n", strerror(errno));
                exit(1);
            }
            waitpid(pid_child, NULL, 0);
        }
        

        break;

    case 4: // Pipe
        pid_t pid_w, pid_r;
        int pfds[2], pipeStatus;
        pipeStatus = pipe(pfds);
        if (pipeStatus == -1)
        {
            fprintf(stderr, "pipe failed, error: %s\n", strerror(errno));
            exit(1);
        }
        arglist[pipeIndex] = NULL; // Replace the pipe character with NULL

        // Split the arglist into two arrays for the two commands - execvp uses NULL terminated arrays
        char **firstCommand = arglist;
        char **secondCommand = &arglist[pipeIndex + 1];

        pid_w = fork();
        if (pid_w == 0)
        {
            dup2(pfds[1], STDOUT); // Redirect the write end of the pipe to stdout
            // Close the read and write ends of the pipe
            close(pfds[0]); 
            close(pfds[1]); 
            execvp(firstCommand[0], firstCommand);
            exit(1); // check if error handling is correct
        }
        else
        {
            pid_r = fork();
            if (pid_r == 0)
            {
                dup2(pfds[0], STDIN); // Redirect the read end of the pipe to stdin
                // Close the read and write ends of the pipe
                close(pfds[0]); 
                close(pfds[1]); 
                execvp(secondCommand[0], secondCommand);
                exit(1); // check if error handling is correct
            }
            else
            {
                if (pid_w == -1 || pid_r == -1)
                {
                    fprintf(stderr, "one or more fork failed, error: %s\n", strerror(errno));
                    exit(1);
                }
                
                close(pfds[0]);
                close(pfds[1]);
                waitpid(pid_w, NULL, 0);
                waitpid(pid_r, NULL, 0);
            }
        }
        break;
    
    default:
        break;
    }
    
}

int finalize(void)
{
    return 0;
}
